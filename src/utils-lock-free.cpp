#include "red-black-lock-free.h"

using namespace std;

/* Helper functions for lock-free */
void clear_local_area_insert(TreeNode &node, vector<TreeNode> &flagged_nodes) {
  // Release all flags in the local area
  // First, reset all flags to false
  // printf("%ld flagged nodes\n", flagged_nodes.size());
  for (auto &node : flagged_nodes) {
    // printf("Setting flag of %d to false\n", node->val);
    // No need to do null check since we know all nodes in flagged_nodes exist
    node->flag = false;
  }

  // Then, clear all markers on up to 4 ancestors (including gp) and reset to -1
  TreeNode p = node->parent;
  if (!p) return;
  // TreeNode marker_node = p->parent;
  
  /* for (int i = 0; i < 4; i++) {
    if (marker_node) {
      marker_node->marker = -1;
      marker_node = marker_node->parent;
    }
  }*/ 
}


bool get_markers_above_delete(TreeNode start, TreeNode node_ignore, bool release) {
  bool expected = false;
  int thread_id = omp_get_thread_num();

  // Get flags to set markers myself without interference
  TreeNode prev = start;
  TreeNode cur = start->parent;
  for (int i = 0; i < 4; i++) {
    if (cur && cur != node_ignore) {
      if (!cur->flag.compare_exchange_weak(expected, true)) {
        return false;
      }
    }
    if (cur != prev->parent || \
      (cur->marker != DEFAULT_MARKER && cur->marker != thread_id)) {
      // TODO: CHECK 2nd condition
      for (TreeNode tmp = start; tmp != cur->parent; tmp = tmp->parent) {
        if (tmp) {
          tmp->flag = false;
        }
      }
      return false;
    }
    prev = cur;
    cur = cur->parent;
  }

  // Set Markers now that you have the flags
  cur = start->parent;
  for (int i = 0; i < 4; i++) {
    // TODO: NULL CHECK
    cur->marker = omp_get_thread_num();
    cur = cur->parent;
  }

  // Remove flags if u want
  if (release) {
    cur = start->parent;
    for (int i = 0; i < 4; i++) {
      if (cur && cur != node_ignore) {
        cur->flag = false;
        cur = cur->parent;
      }
    }
  }
  return true;
}


void clear_local_area_delete(TreeNode &node, vector<TreeNode> &flagged_nodes) {
  // Release all flags in the local area

  // Reset all flags to false
  for (auto &node : flagged_nodes) {
    node->flag = false;
  }
  
  // Then, clear all markers on up to 4 ancestors (including gp) and reset to -1
  if (!node) return; // If node is root, no need to clear markers

  int thread_id = omp_get_thread_num();

  TreeNode marker_node = node->parent;
  for (int i = 0; i < 4; i++) {
    if (marker_node && marker_node->marker == thread_id) {
      marker_node->marker = -1;
      marker_node = marker_node->parent;
    }
  }
}


/* Helper functions for insert */
bool setup_local_area_insert(TreeNode &node, vector<TreeNode> &flagged_nodes) {
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
    } else if (node != nullptr) {
      flagged_nodes.push_back(node);
    }
  }

  // int thread_id = omp_get_thread_num();

  // Setup Markers
  /*TreeNode marker_node = gp;
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
  }  */
  return true;
}

bool setup_local_area_delete(TreeNode &start, TreeNode &node, vector<TreeNode> &flagged_nodes) {
  // TODO: CHECK correctness - CHECK SIMILARITY
  // Note: node and p are guaranteed to exist, the other nodes in the local area are not
  printf("setup_local_area_delete\n");

  TreeNode x = start->child[!start->child[0]]; // Get the only child of start - guaranteed to exist

  bool expected = false;
  printf("Start: %d, Node: %d\n, left child %p, right child %p\n", start->val, start->val, start->child[0], start->child[1]);

  if (x && !x->flag.compare_exchange_weak(expected, true)) {
    printf("Failed to set flag of x\n");
    return false;
  }
  if (!x) printf("x is null\n");

  TreeNode p = start->parent;
  if (!p) printf("p is null\n");

  // Try setting the flag of the parent and ensure p is not the node we're trying to delete
  if (p && p != node && !(p->flag.compare_exchange_weak(expected, true))) {
    x->flag = false;
    return false;
  }

  // This checks if the parent remains the same
  if (p != start->parent) {
    if (p != node) p->flag = false;
    x->flag = false;
    return false;
  }

  TreeNode w = p->child[p->child[1] != node];

  if (!w->flag.compare_exchange_weak(expected, true)) {
    if (p != node) p->flag = false;
    x->flag = false;
    return false;
  }

  TreeNode wlc = nullptr, wrc = nullptr;
  if (w) {
    wlc = w->child[0];
    wrc = w->child[1];

    if (wlc && !wlc->flag.compare_exchange_weak(expected, true)) {
      if (p != node) p->flag = false;
      w->flag = false;
      x->flag = false;
      return false;
    }

    if (wrc && !wrc->flag.compare_exchange_weak(expected, true)) {
      if (p != node) p->flag = false;
      w->flag = false;
      wlc->flag = false;
      x->flag = false;
      return false;
    }
  }

  // Check the four markers above the parent node by setting the flags
  if (!get_markers_above_delete(p, node, true)) {
    if (x) x->flag = false;
    if (w) {
      w->flag = false;
      wlc->flag = false;
      wrc->flag = false;
    }
    if (p != node) p->flag = false;
    return false;
  }

  flagged_nodes.push_back(x);
  flagged_nodes.push_back(w);
  flagged_nodes.push_back(p);
  if (w) {
    flagged_nodes.push_back(wlc);
    flagged_nodes.push_back(wrc);
  }



  // TreeNode p = nullptr, w = nullptr, wlc = nullptr, wrc = nullptr;
  // TreeNode x = start->child[!start->child[0]]; // Get the only child of start - guaranteed to exist
  // if (start) p = start->parent;
  // if (p) w = p->child[p->child[1] != node];
  // if (w) {
  //   wlc = w->child[0];
  //   wrc = w->child[1];
  // }

  // printf("%d\n", __LINE__);

  // // Setup flags for local area (all 5 nodes)
  // bool expected = false;

  // vector<TreeNode> nodes_to_be_flagged = {x, w, p, wlc, wrc};

  // for (auto &node : nodes_to_be_flagged) {
  //   if (node && (node->marker != -1 || !node->flag.compare_exchange_weak(expected, true))) {
  //     // If the flag couldn't be set correctly, roll back the changes to flags and return false => return to root
  //     for (auto &flagged_node : flagged_nodes) {
  //       if (flagged_node != node) flagged_node->flag = false;
  //     }
  //     flagged_nodes.clear();

  //     return false;
  //   } else if (node != nullptr) {
  //     flagged_nodes.push_back(node);
  //     printf("Node pushed\n");
  //   }
  // }
  // printf("%d\n", __LINE__);


  // int thread_id = omp_get_thread_num();

  // if (!p) return true; // If node is root, no need to set markers
  // printf("%d - p (%p), gp (%p)\n", __LINE__, p, p->parent);
  // TreeNode gp = p->parent;
  // TreeNode marker_node = gp;
  // printf("%d\n", __LINE__);


  // // Get 4 markers above the parent node
  // for (int i = 0; i < 4; i++) {
  //   if (marker_node) {
  //     if (marker_node->marker == -1) {
  //       marker_node->marker = thread_id;
  //       marker_node = marker_node->parent;
  //     } else {
  //       // Writing to the i-th marker failed
  //       // Reset all flags
  //       for (auto &flagged_node : flagged_nodes) {
  //         if (flagged_node && flagged_node != node) flagged_node->flag = false;
  //       }
        
  //       // Reset all markers up to the (i-1)-th marker (inclusive)
  //       marker_node = gp;
  //       for (int j = 0; j < i - 1; j++) {
  //         if (marker_node) {
  //           marker_node->marker = -1;
  //           marker_node = marker_node->parent;
  //         }
  //       }

  //       return false;
  //     }
  //   }
  // }  
  // printf("%d\n", __LINE__);

  return true;
}

// Move Local Area Up when insert needs to go up a layer
// NOTE: `node` here is the original node BEFORE moveup, not after!
void move_local_area_up_insert(TreeNode &node, vector<TreeNode> &flagged_nodes) {
  // printf("Moving local area up\n");
  // Define Parent, Grandparent, and Uncle of Original Node
  TreeNode p = node->parent;
  TreeNode gp = p->parent;
  TreeNode u = gp->child[gp->child[1] != p];
  // printf("%d\n", __LINE__);

  // Remove flag from node, parent, and uncle
  // By virtue of moving up to gp, we know p and gp must already exist
  node->flag = false;
  p->flag = false;
  if (u) u->flag = false;

  // Remove nodes from flagged_nodes
  for (size_t i = 0; i < flagged_nodes.size(); i++) {
    if (flagged_nodes[i] == node) {
      flagged_nodes.erase(flagged_nodes.begin() + i);
    }
    if (flagged_nodes[i] == p) {
      flagged_nodes.erase(flagged_nodes.begin() + i);
    }
    if (u && flagged_nodes[i] == u) {
      flagged_nodes.erase(flagged_nodes.begin() + i);
    }
  }

  // printf("%d\n", __LINE__);



  // Set node to grandparent, and return if new node is the root
  node = gp;
  if (!node->parent) {
    return;
  }

  // Remove the markers for old grandparent and greatgrandparent (new node and parent)
  // Also add flags for all except new node (who already had one as old gp)
  // printf("%d\n", __LINE__);

  bool expected = false;
  p = node->parent;
  gp = p->parent;
  // node->marker = -1;
  // printf("%d\n", __LINE__);

  if (gp) u = gp->child[gp->child[1] != p];
  // printf("%d\n", __LINE__);
  
  while (!p->flag.compare_exchange_weak(expected, true));
  flagged_nodes.push_back(p);
  // p->marker = -1;
  // printf("%d\n", __LINE__);

  if (gp) {
    while (!gp->flag.compare_exchange_weak(expected, true));
    flagged_nodes.push_back(gp);
  }

  // printf("%d\n", __LINE__);

  if (u) {
    while (u->marker != -1 && !u->flag.compare_exchange_weak(expected, true));
    flagged_nodes.push_back(u);
  }
  // printf("%d\n", __LINE__);
  
  // Add new markers for new gp and above
  /*int thread_id = omp_get_thread_num();
  TreeNode marker_node = gp;
  for (int i = 0; i < 4; i++) { 
    if (marker_node) {
      while (marker_node->marker != -1 && marker_node->marker != thread_id);
      // TODO: (optional) implement backoff to avoid busy waiting
      marker_node->marker = thread_id;
      marker_node = marker_node->parent;
    }
  }*/
}


bool get_flags_and_markers_above_delete(TreeNode &node) {
  // Set four markers above the parent node and an additional marker for moving up during delete
  if (!get_markers_above_delete(node, nullptr, false)) { // TODO: DELETE THIS LATER, 2nd arg incorrect
    return false;
  }

  int thread_id = omp_get_thread_num();

  bool expected = false;
  vector<TreeNode> flagged_nodes;
  TreeNode curr = node;

  for (int i = 0; i < 4; i++) {
    curr = curr->parent;
    flagged_nodes.push_back(curr);
  }

  curr = curr->parent;

  if (!curr->flag.compare_exchange_weak(expected, true)) {
    for (auto &node : flagged_nodes) {
      node->flag = false;
    }
    flagged_nodes.clear();
    return false;
  }

  if (curr != flagged_nodes[3]->parent || (curr != node && curr->marker != DEFAULT_MARKER && curr->marker != thread_id)) {
    for (auto &node : flagged_nodes) {
      node->flag = false;
    }
    flagged_nodes.clear();
    curr->flag = false;
    return false;
  }

  curr->marker = thread_id;
  flagged_nodes[1]->flag = false;

  return true;
}

bool set_moved_local_area_delete(TreeNode new_sibling, TreeNode new_sibling_lc, TreeNode new_sibling_rc) {
  bool expected = false;
  if (!new_sibling->flag.compare_exchange_weak(expected, true)) {
    return false;
  }
  if (!new_sibling_lc->flag.compare_exchange_weak(expected, true)) {
    new_sibling->flag = false;
    return false;
  }
  if (!new_sibling_rc->flag.compare_exchange_weak(expected, true)) {
    new_sibling->flag = false;
    new_sibling_lc->flag = false;
    return false;
  }
  return true;
}

// Move Local Area Up when delete needs to go up a layer
// NOTE: `node` here is the original node BEFORE moveup, not after!
// Called in delete_case_4 (which corresponds to fix up case 2 where both nephews are black)
void move_local_area_up_delete(TreeNode &node, vector<TreeNode> &flagged_nodes) {
  printf("move_local_area_up_delete\n");
  TreeNode p = node->parent; // old parent
  TreeNode w = p->child[p->child[1] != node];
  TreeNode wlc = w->child[0];
  TreeNode wrc = w->child[1];
  printf("%d\n", __LINE__);
  
  while (!get_flags_and_markers_above_delete(node));
  
  printf("%d\n", __LINE__);

  TreeNode new_node = p, new_parent = p->parent;
  TreeNode new_sibling = new_parent->child[new_parent->child[1] != new_node];
  TreeNode new_sibling_lc = new_sibling->child[0], new_sibling_rc = new_sibling->child[1];

  printf("%d\n", __LINE__);

  // Move the following to a separate function

  // Set the flags of the new local area
  // for (auto &curr : flagged_nodes) {
  //   if (curr != node) curr->flag = false;
  // }
  // Try ONE of above OR below
  while (!set_moved_local_area_delete(new_sibling, new_sibling_lc, new_sibling_rc));

  printf("%d\n", __LINE__);

  p->flag = false;
  w->flag = false;
  wlc->flag = false;
  wrc->flag = false;

  printf("%d\n", __LINE__);

  new_parent->marker = DEFAULT_MARKER;

  printf("%d\n", __LINE__);

  // Record the changes to the new local area
  flagged_nodes.clear();
  flagged_nodes.push_back(new_node);
  flagged_nodes.push_back(new_sibling);
  flagged_nodes.push_back(new_parent);
  flagged_nodes.push_back(new_sibling_lc);
  flagged_nodes.push_back(new_sibling_rc);
}


void tree_to_vec(TreeNode &node, vector<int> &vec, vector<int> &flags, vector<int> &markers) {
  if (!node) return;
  tree_to_vec(node->child[0], vec, flags, markers);
  vec.push_back(node->val);
  flags.push_back(node->flag);
  markers.push_back(node->marker);
  tree_to_vec(node->child[1], vec, flags, markers);
}


void print_tree(TreeNode &node) {
  printf("---------------Printing tree---------------\n");
  std::vector<int> vec, flags, markers;
  tree_to_vec(node, vec, flags, markers);
  for (size_t i = 0; i < vec.size(); i++) {
    printf("%d, flag %d, marker %d\n", vec[i], flags[i], markers[i]);
  }
  printf("\n");
}