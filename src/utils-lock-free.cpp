#include "red-black-lock-free.h"

using namespace std;

/* Helper functions for lock-free */
void clear_local_area_insert(TreeNode &node, vector<TreeNode> flagged_nodes) {
  // Release all flags in the local area
  // First, reset all flags to false
  for (auto &node : flagged_nodes) {
    node->flag = false;
  }

  // Then, clear all markers on up to 4 ancestors (including gp) and reset to -1
  TreeNode p = node->parent;
  if (!p) return;
  TreeNode marker_node = p->parent;
  
  for (int i = 0; i < 4; i++) {
    if (marker_node) {
      marker_node->marker = -1;
      marker_node = marker_node->parent;
    }
  }
}


void clear_local_area_delete(TreeNode &node, vector<TreeNode> flagged_nodes) {
  // Release all flags in the local area
  TreeNode p = nullptr, w = nullptr, wlc = nullptr, wrc = nullptr;

  // Reset all flags to false
  for (auto &node : flagged_nodes) {
    node->flag = false;
  }
  
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
bool setup_local_area_insert(TreeNode &node, vector<TreeNode> flagged_nodes) {
  // Note: node and p are guaranteed to exist, gp and u are not
  TreeNode p = nullptr, gp = nullptr, u = nullptr;
  if (node) p = node->parent;
  if (p) gp = p->parent;
  if (gp) u = gp->child[gp->child[1] != p];

  // Set up Flags
  bool expected = false;

  vector<TreeNode> nodes_to_be_flagged = {node, p, gp, u};

  for (auto &node : nodes_to_be_flagged) {
    if (node && (node->marker != -1 || !node->flag.compare_exchange_weak(expected, true))) {
      // If the flag couldn't be set correctly, roll back the changes to flags and return false => return to root
      for (auto &flagged_node : flagged_nodes) {
        flagged_node->flag = false;
      }
      return false;
    } else {
      flagged_nodes.push_back(node);
    }
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
        if (gp) gp->flag = false;   
        if (u) u->flag = false;
        
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

bool setup_local_area_delete(TreeNode &node, vector<TreeNode> flagged_nodes) {
  // Note: node and p are guaranteed to exist, the other nodes in the local area are not
  TreeNode p = nullptr, w = nullptr, wlc = nullptr, wrc = nullptr;
  if (node) p = node->parent;
  if (p) w = p->child[p->child[1] != node];
  if (w) {
    wlc = w->child[0];
    wrc = w->child[1];
  }

  // Setup flags for local area (all 5 nodes)
  bool expected = false;

  vector<TreeNode> nodes_to_be_flagged = {node, p, w, wlc, wrc};

  for (auto &node : nodes_to_be_flagged) {
    if (node && (node->marker != -1 || !node->flag.compare_exchange_weak(expected, true))) {
      // If the flag couldn't be set correctly, roll back the changes to flags and return false => return to root
      for (auto &flagged_node : flagged_nodes) {
        flagged_node->flag = false;
      }
      return false;
    } else if (node) {
      flagged_nodes.push_back(node);
    }
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
  // By virtue of moving up to gp, we know p and gp must already exist
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

void tree_to_vec(TreeNode &node, vector<int> &vec, vector<int> &flags) {
  if (!node) return;
  tree_to_vec(node->child[0], vec, flags);
  vec.push_back(node->val);
  flags.push_back(node->flag);
  tree_to_vec(node->child[1], vec, flags);
}

void print_tree(TreeNode &node) {
  std::vector<int> vec, flags;
  tree_to_vec(node, vec, flags);
  for (int i = 0; i < vec.size(); i++) {
    printf("%d, flag %d\n", vec[i], flags[i]);
  }
  printf("\n");
}