#include "red-black-lock-free.h"
#include <stdlib.h>

/* Helper functions for lock-free */
void clear_local_area_insert(TreeNode &node) {
  // Release all flags in the local area
  TreeNode p = nullptr, gp = nullptr, u = nullptr;

  // First, reset all flags to false
  if (node) {
    node->flag = false;
    p = node->parent;
  }
  if (p) {
    p->flag = false;
    gp = p->parent;
  }
  if (gp) {
    u = gp->child[gp->child[1] != p];
    if (u) u->flag = false;
  }

  // Then, clear all markers on up to 4 ancestors (including gp) and reset to -1
  TreeNode marker_node = gp;
  for (int i = 0; i < 4; i++) {
    if (marker_node) {
      marker_node->marker = -1;
      marker_node = marker_node->parent;
    }
  }
}


void clear_local_area_delete(TreeNode &node) {
  // Release all flags in the local area
  TreeNode p = nullptr, w = nullptr, wlc = nullptr, wrc = nullptr;

  // First, reset all flags to false
  if (node) {
    node->flag = false;
    p = node->parent;
  }
  if (p) {
    p->flag = false;
    w = p->child[p->child[1] != node];
  }
  if (w) {
    w->flag = false;
    wlc = w->child[0];
    wrc = w->child[1];
  }
  if (wlc) wlc->flag = false;
  if (wrc) wrc->flag = false;
  
  // Then, clear all markers on up to 4 ancestors (including gp) and reset to -1
  if (!p) return; // If node is root, no need to clear markers

  TreeNode marker_node = p->parent;
  for (int i = 0; i < 4; i++) {
    if (marker_node) {
      marker_node->marker = -1;
      marker_node = marker_node->parent;
    }
  }
}

void is_in_local_area(TreeNode &node) {}

/* Helper functions for insert */
bool setup_local_area_insert(TreeNode &node) {
  TreeNode p = nullptr, gp = nullptr, u = nullptr;
  if (node) p = node->parent;
  if (p) gp = p->parent;
  if (gp) u = gp->child[gp->child[1] != p];

  // Setup Flags
  bool expected = false;
  // TODO: do nullptr check for the nodes below
  if (node->marker != -1 || !node->flag.compare_exchange_weak(expected, true)) {
    return false;
  }
  if (p->marker != -1 || !p->flag.compare_exchange_weak(expected, true)) {
    node->flag = false;
    return false;
  }
  if (gp->marker != -1 || !gp->flag.compare_exchange_weak(expected, true)) {
    node->flag = false;
    p->flag = false;
    return false;
  }
  if (u->marker != -1 || !u->flag.compare_exchange_weak(expected, true)) {
    node->flag = false;
    p->flag = false;
    gp->flag = false;
    return false;
  }

  int thread_id = omp_get_thread_num();

  // Setup Markers
  TreeNode marker_node = gp;
  for (int i = 0; i < 4; i++) {
    if (marker_node) {
      if (marker_node->marker == -1) {
        marker_node->marker = thread_id;
        marker_node = marker_node->parent;
      } else {
        // Writing to the i-th marker failed
        // Reset all flags
        p->flag = false;
        gp->flag = false;   
        u->flag = false;
        
        // Reset all markers up to the (i-1)-th marker (inclusive)
        marker_node = gp;
        for (int j = 0; j < i - 1; j++) {
          if (marker_node) {
            marker_node->marker = -1;
            marker_node = marker_node->parent;
          }
        }

        return false;
      }
    }
  }  
  return true;
}

bool setup_local_area_delete(TreeNode &node) {
  TreeNode p = nullptr, w = nullptr, wlc = nullptr, wrc = nullptr;
  if (node) p = node->parent;
  if (p) w = p->child[p->child[1] != node];
  if (w) {
    wlc = w->child[0];
    wrc = w->child[1];
  }

  // Setup flags for local area (all 5 nodes)
  // TODO: do nullptr check for the nodes below
  bool expected = false;
  if (node->marker != -1 || !node->flag.compare_exchange_weak(expected, true)) {
    return false;
  }
  if (p->marker != -1 || !p->flag.compare_exchange_weak(expected, true)) {
    node->flag = false;
    return false;
  }
  if (w->marker != -1 || !w->flag.compare_exchange_weak(expected, true)) {
    node->flag = false;
    p->flag = false;
    return false;
  }
  if (wlc->marker != -1 || !wlc->flag.compare_exchange_weak(expected, true)) {
    node->flag = false;
    p->flag = false;
    w->flag = false;
    return false;
  }
  if (wrc->marker != -1 || !wrc->flag.compare_exchange_weak(expected, true)) {
    node->flag = false;
    p->flag = false;
    w->flag = false;
    wlc->flag = false;
    return false;
  }

  int thread_id = omp_get_thread_num();

  if (!p) return true; // If node is root, no need to set markers
  TreeNode gp = p->parent;
  TreeNode marker_node = gp;

  // Get 4 markers above the parent node
  for (int i = 0; i < 4; i++) {
    if (marker_node) {
      if (marker_node->marker == -1) {
        marker_node->marker = thread_id;
        marker_node = marker_node->parent;
      } else {
        // Writing to the i-th marker failed
        // Reset all flags
        node->flag = false;
        p->flag = false;
        w->flag = false;
        wlc->flag = false;
        wrc->flag = false;
        
        // Reset all markers up to the (i-1)-th marker (inclusive)
        marker_node = gp;
        for (int j = 0; j < i - 1; j++) {
          if (marker_node) {
            marker_node->marker = -1;
            marker_node = marker_node->parent;
          }
        }

        return false;
      }
    }
  }  
  return true;
}

// Move Local Area Up when insert needs to go up a layer
// NOTE: `node` here is the original node BEFORE moveup, not after!
void move_local_area_up(TreeNode &node) {
  // Define Parent, Grandparent, and Uncle of Original Node
  TreeNode p, gp, u = nullptr;
  p = node->parent;
  gp = p->parent;
  u = gp->child[gp->child[1] != p];

  // Remove flag from node, parent, and uncle
  // By virute of moving up to gp, we know p and gp must already exist
  node->flag = false;
  p->flag = false;
  if (u) u->flag = false;


  // Set node to grandparent, and return if new node is the root
  node = gp;
  if (!node->parent) {
    return;
  }

  // Remove the markers for old grandparent and greatgrandparent (new node and parent)
  // Also add flags for all except new node (who already had one as old gp)
  bool expected = false;
  p = node->parent;
  gp = p->parent;
  node->marker = -1;
  if (gp) u = gp->child[gp->child[1] != p];
  while (!p->flag.compare_exchange_weak(expected, true));
  p->marker = -1;
  while (!gp->flag.compare_exchange_weak(expected, true));
  if (u) {
    while (u->marker != -1 && !u->flag.compare_exchange_weak(expected, true));
  }
  
  // Add new markers for new gp and above
  int thread_id = omp_get_thread_num();
  TreeNode marker_node = gp;
  for (int i = 0; i < 4; i++) { 
    if (marker_node) {
      while (marker_node->marker != -1 && marker_node->marker != thread_id);
      marker_node->marker = thread_id;
      marker_node = marker_node->parent;
    }
  }
}

// string operation_to_string(struct Operation operation) {
//   switch (operation.type) {
//     case INSERT:
//       return "INSERT " + std::to_string(operation.val);
//       break;
//     case DELETE:
//       return "DELETE " + std::to_string(operation.val);
//       break;
//     case LOOKUP:
//       return "LOOKUP " + std::to_string(operation.val);
//       break;
//     default:
//       return "INVALID OPERATION";
//   }
// }