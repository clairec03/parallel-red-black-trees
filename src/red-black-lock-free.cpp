#include "utils-lock-free.cpp"
#include <stdio.h>
#include <sched.h>
#include <omp.h>

using namespace std;

/* Invariants of the lock-free implementation */
// 1. `flag` field for each node indicates whether the node is being modified
//    Setting the flag field protects the local area around the node for insertion
//    including the node itself, its parent, uncle, and grandparent by checking flags
// 2. Intention markers ensure a safe distance between any two processes s.t.
//    each process is guaranteed to be able to act. An intention marker is an integer field
//    in each node that is set by a process to indicate that the process intends to 
//    move its local area up to that node. To set a marker, a thread places its thread ID 
//    in the marker field of the node (omp_get_thread_num();). After the work in the local area
//    is done, the thread resets its own markers and flags.

/* Rotations cause the marker locations to change */
// 1. For the insertion case, simply remove markers when done
// 2. For the deletion case, the markers must be moved

inline TreeNode newTreeNode(int val, bool red, TreeNode parent, 
                            TreeNode left, TreeNode right) {
  TreeNode node = new struct RedBlackNode();
  node->child[0] = left;
  node->child[1] = right;
  node->parent = parent;
  node->val = val;
  node->marker = -1; // Initialized to -1
  node->flag = false;
  node->red = red;
  return node;
}

TreeNode rotateDir(Tree &tree, TreeNode &root, int dir) {
  TreeNode parent = root->parent;
  TreeNode rotatingChild = root->child[1-dir];
  // assert(rotatingChild);
  TreeNode C = rotatingChild->child[dir];
  root->child[1-dir] = C;
  if (C) {
    C->parent = root;
  }
  rotatingChild->child[dir] = root;
  
  root->parent = rotatingChild;
  rotatingChild->parent = parent;
  if (parent) {
    parent->child[root == parent->child[1]] = rotatingChild;
  } else {
    tree->root = rotatingChild;
  }
  return rotatingChild;
}

Tree tree_init() {
  Tree tree = new struct RedBlackTree();
  tree->root = nullptr;
  return tree;
}



string subtreeToString(TreeNode root) {
  if (!root) {
    return "Empty";
  }
  if (root->red) 
    return "RED(" + subtreeToString(root->child[0]) + ", " + to_string(root->val) + ", " + subtreeToString(root->child[1]) + ")";
  else 
    return "BLACK(" + subtreeToString(root->child[0]) + ", " + to_string(root->val) + ", " + subtreeToString(root->child[1]) + ")";
}

string tree_to_string(Tree T) {
  return subtreeToString(T->root);
}

int size_subtree(TreeNode &root) {
  if (!root) return 0;
  return 1 + size_subtree(root->child[0]) + size_subtree(root->child[1]);
}

void inord_tree_to_vec_helper(TreeNode T, vector <int> &res) {
  if (!T) return;
  inord_tree_to_vec_helper(T->child[0], res);
  res.push_back(T->val);
  inord_tree_to_vec_helper(T->child[1], res);
}

vector <int> tree_to_vector(Tree &T) {
  TreeNode root = T->root;
  vector <int> res;
  if (!root) return res;
  
  inord_tree_to_vec_helper(root, res);

  return res;
}

int tree_size(Tree &tree) {
  return size_subtree(tree->root);
}

// Return Whether Red-Black Tree Rooted at root is valid
// If it is valid, also return the number of black nodes to any Empty,
// including this info allows validation to be written recursively
bool validateAtBlackDepth(TreeNode &root, int *blackDepth, int *lo, int *hi) {
  // (Base Case) Leaves are Valid
  if (!root) {
    *blackDepth = 0;
    return true;
  }

  // Root must follow BST invariant
  if (lo && root->val <= *lo || hi && *hi <= root->val) {
    printf("BST Invariant Failed at %d! \n", root->val);
    return false;
  }

  // Red Nodes Cannot have Red Children
  TreeNode left = root->child[0], right = root->child[1];
  if (root->red && ((left && left->red) || (right && right->red))) {
    printf("Red Children Invariant Failed at %d! \n", root->val);
    return false;
  }

  // Children Must Point back to their Parents
  if ((left && left->parent != root) || (right && right->parent != root)) {
    printf("Orphaned Children at %d! \n", root->val);
    return false;
  }

  // Left and right subtrees must be valid red-black trees
  int leftDepth = 0, rightDepth = 0;
  bool leftValid = validateAtBlackDepth(left, &leftDepth, lo, &(root->val));
  bool rightValid = validateAtBlackDepth(right, &rightDepth, &(root->val), hi);

  if (!leftValid || !rightValid) {
    return false;
  }

  // Black depth must be the same for both children
  if (leftDepth != rightDepth) {
    printf("Black Depth Invariant Failed at %d! \n", root->val);
    return false;
  }
  
  *blackDepth = leftDepth + !(root->red);
  return true;
}

// Return whether Red-Black Tree Rooted at root is valid
bool tree_validate(Tree &tree) {
  if (tree->root && tree->root->parent) {
    printf("Root has a parent!\n");
    return false;
  }
  int blackDepth = 0;
  return validateAtBlackDepth(tree->root, &blackDepth, nullptr, nullptr);
}

TreeNode tree_find(Tree &tree, int val) {
  // TODO
  return nullptr;
}

// Return whether a node with given value exists in a Red-Black Tree
bool tree_lookup(Tree &tree, int val) {
  TreeNode node = tree->root;
  while (node) {
    TreeNode old_node = node;
    if (val < node->val) {
      node = node->child[0];
    } else if (val > node->val) {
      node = node->child[1];
    } else {
      return true;
    }
    bool expected = false;
    if (!node) {
      return false;
    }
    old_node->flag = false;
    if (!node->flag.compare_exchange_weak(expected, true)) {
      // TODO for extension goal - Backoff???
      return tree_lookup(tree, val);
    }
  }
  // should never reach here
  return false;
}

// Inserts Node into Tree, returns True if Node Inserted (i.e. wasn't already present)
bool tree_insert(Tree &tree, int val) {
  vector<TreeNode> flagged_nodes;
  printf("INSERTING %d\n", val);
  // Edge Case: Set root of Empty tree
  if (!tree->root) {
    tree->root = newTreeNode(val, true, nullptr, nullptr, nullptr);
    return true;
  }
  printf("%d\n", __LINE__);
  print_tree(tree->root);
  
  // TODO: Search down to find where node would be
  TreeNode iter = tree->root;
  TreeNode parent = tree->root->parent;

  while (iter) {
    parent = iter;

    if (val == iter->val) {
      return false;
    } else {
      iter = iter->child[(val > iter->val)];
      // Insert into left child if true, right child if false
    }
  }

  // Place Node Where it Would be in the Tree Assuming No Rebalancing
  TreeNode node = newTreeNode(val, true, parent, nullptr, nullptr);
  
  printf("%d\n", __LINE__);
  print_tree(tree->root);
  if (!setup_local_area_insert(node, flagged_nodes)) {
    fprintf(stderr, "Failed to setup local area for insert\n"); 
    // This should never be printed for n = 1
    exit(1); // TODO: remove this line for n > 1
    return tree_insert(tree, val);
  }
  printf("flagged_nodes size: %ld in calling function\n", flagged_nodes.size());
  if (val < parent->val) {
    parent->child[0] = node;
  } else {
    parent->child[1] = node;
  }
  printf("%d\n", __LINE__);  
  // Go Through the Cases of Tree Insertion
  // Source: https://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Insertion
  TreeNode grandparent;
  TreeNode uncle;
  int dir;
  while (node->parent) {
    // If Parent is Black, Chilling (I1)
    if (!parent->red) {
      clear_local_area_insert(node, flagged_nodes);
      return true;
    }
    printf("%d\n", __LINE__);
    // If Parent is Red Root, Turn Black and Return (I4)
    grandparent = parent->parent;
    if (!grandparent) {
      parent->red = false;
      clear_local_area_insert(node, flagged_nodes);
      return true;
    }
    printf("%d\n", __LINE__);
    // Define Uncle as Grandparent's Other Child
    dir = parent->val > grandparent->val;
    uncle = grandparent->child[1-dir];
    if (!uncle || !uncle->red) {
      // (I5 & I6)
      if (node == parent->child[1-dir]) {
        rotateDir(tree, parent, dir);
        node = parent;
        parent = grandparent->child[dir];
      }
      printf("%d\n", __LINE__);
      rotateDir(tree, grandparent, 1-dir);
      parent->red = false;
      grandparent->red = true;
      // A little bit suspicious about this but idk
      // Like should local area change when we do rotations?
      // Since we're not storing as vectors, so flags will no longer be at same areas
      clear_local_area_insert(node, flagged_nodes);
      return true;
    }
    printf("%d\n", __LINE__);
    // Case parent and uncle are both red nodes

    // Parent and Uncle Both Red, Swap Parent + Grandparent Colors (I2)
    parent->red = false;
    uncle->red = false;
    grandparent->red = true;

    node = grandparent;
    // Move local area up to the grandparent
    move_local_area_up_insert(node); // TODO: Correctness check
    parent = node->parent;
    printf("%d\n", __LINE__);
  }
  printf("%d\n", __LINE__);
  // If We're the Root, Done (I3)
  clear_local_area_insert(node, flagged_nodes);
  printf("%d\n", __LINE__);
  return true;
}

// Runs parallel insert on values
void tree_insert_bulk(Tree &tree, vector<int> values, int batch_size, int num_threads) {
  int num_operations = values.size();
  #pragma omp parallel for schedule(static, batch_size) num_threads(num_threads)
  for (int i = 0; i < num_operations; i++) {
    printf("%d\n", __LINE__);
    tree_insert(tree, values[i]);
  }
  return;
}

// HELPER FUNCTIONS FOR DELETE (As per Wikipedia)
bool delete_case_6(Tree &tree, TreeNode parent, TreeNode sibling, TreeNode distant_nephew, int dir, vector<TreeNode> &flagged_nodes) {
  // Fix up case 4 - do nothing
  rotateDir(tree, parent, dir);
  sibling->red = parent->red;
  parent->red = false;
  distant_nephew->red = false;
  clear_local_area_delete(parent, flagged_nodes);
  return true;
}

bool delete_case_5(Tree &tree, TreeNode node, TreeNode parent, TreeNode sibling, 
                   TreeNode close_nephew, TreeNode distant_nephew, int dir,
                   vector<TreeNode> &flagged_nodes) {
  // Close nephew is red, distant nephew is black (possibly leaf node)
  close_nephew->red = false;
  sibling->red = true;
  rotateDir(tree, sibling, 1-dir);
  distant_nephew = sibling;
  sibling = close_nephew; // after rotation, the close nephew becomes sibling (parent->child[1-dir])
  close_nephew = sibling->child[dir];

  // Fix up case 3 - CHECK
  TreeNode oldw = sibling->child[1-dir];
  TreeNode old_disant_nephew = oldw->child[1-dir]; 

  // Clear markers in the original local area
  for (auto &node : flagged_nodes) {
    node->marker = -1;
  }

  sibling->child[dir]->flag = true;
  old_distant_nephew->flag = false;

  // Reset the nodes in the local area after rotation
  flagged_nodes.clear();
  flagged_nodes.push_back(node);
  flagged_nodes.push_back(parent);
  flagged_nodes.push_back(sibling);
  flagged_nodes.push_back(close_nephew);
  flagged_nodes.push_back(distant_nephew);
  // End of fix up case 3

  return delete_case_6(tree, parent, sibling, distant_nephew, dir, flagged_nodes);
}

bool delete_case_4(TreeNode &sibling, TreeNode &parent, vector<TreeNode> &flagged_nodes) {
  // Fix up case 2 - both nephews are black
  sibling->red = true;
  parent->red = false;
  // TODO: node = move_deleter_up(node);

  // Start of moving deleter up


  // End of moving deleter up

  clear_local_area_delete(parent, flagged_nodes);
  return true;
}

bool delete_case_3(Tree &tree, TreeNode node, TreeNode parent, TreeNode sibling, 
                   TreeNode close_nephew, TreeNode distant_nephew, int dir,
                   vector<TreeNode> &flagged_nodes) {
  rotateDir(tree, parent, dir);
  parent->red = true;
  sibling->red = false;
  sibling = close_nephew; // after rotation, the close nephew becomes sibling (parent->child[1-dir])
  distant_nephew = sibling->child[1-dir];
  close_nephew = sibling->child[dir];

  // now: P red && S black
  // Fixup case 1 - the old sibling is now the new grandparent - CHECK
  // New grandparent is not in the local area, so clear its flag and set its marker to thread id  
  TreeNode gp = parent->parent; // old sibling
  TreeNode old_close_nephew = gp->child[dir], old_distant_nephew = gp->child[1-dir];

  // Clear markers
  if (gp->marker != -1 && gp->marker == gplc->marker) {
    parent->marker = gp->marker;
  }
  
  // Set grandparent's marker, then clear the flag
  gp->marker = omp_get_thread_num();
  gp->flag = false;
  old_distant_nephew->flag = false;

  // Since the old sibling has been moved up, the four flags markers have been shifted up
  // The previous fourth marker is now 5 degrees away - clear it
  TreeNode ancestor = gp;
  for (int i = 0; i < 4; i++) {
    ancestor = ancestor->parent;
  }
  if (ancestor)
    ancestor->marker = -1; // Clear the fifth marker

  // The new nephews are now in the local area, so set their flags to true
  distant_nephew->flag = true;
  close_nephew->flag = true;

  // Reset the nodes in the local area after rotation
  flagged_nodes.clear();
  flagged_nodes.push_back(node);
  flagged_nodes.push_back(parent);
  flagged_nodes.push_back(sibling);
  flagged_nodes.push_back(close_nephew);
  flagged_nodes.push_back(distant_nephew);
  // End of fixup case 1

  if (distant_nephew && distant_nephew->red)
    // Fixup case 4 - do nothing
    return delete_case_6(tree, parent, sibling, distant_nephew, dir, flagged_nodes);
  
  if (close_nephew && close_nephew->red)
    // Fixup case 3
    return delete_case_5(tree, node, parent, sibling, close_nephew, distant_nephew, dir, flagged_nodes);
  
  // Both nephews are black - fixup case 2
  return delete_case_4(sibling, parent, flagged_nodes);
}

bool tree_delete(Tree &tree, int val) {
  // Don't delete from an empty tree
  if (!tree->root) {
    return false;
  }

  // Search down to find where node would be
  TreeNode node;
  TreeNode iter = tree->root;
  TreeNode parent = tree->root->parent;

  while (iter) {
    if (val == iter->val) {
      break;
    } else {
      // Delete from left child if true, right child if false
      parent = iter;
      iter = iter->child[(val > iter->val)];
    }
  }
  // If we never found the node to delete, don't delete it
  node = iter;
  if (!node) {
    return false;
  }

  // Two Node Case
  if (node->child[0] && node->child[1]) {
    // Find in-order successor of Node
    iter = node->child[1];
    while (iter->child[0]) {
      iter = iter->child[0];
    }  
    node->val = iter->val;
    node = iter;
    parent = node->parent;
  }
  
  TreeNode left_child = node->child[0];
  TreeNode right_child = node->child[1];
  TreeNode child = left_child ? left_child : right_child;

  vector<TreeNode> flagged_nodes;

  // One Node Case
  if (child) {
    if (!setup_local_area_delete(node, flagged_nodes)) {
      return tree_delete(tree, val);
    }

    // Replace Node with its extant child
    if (parent) {
      // Node had parent, set parent's child
      bool dir = parent->child[1] == node;
      parent->child[dir] = child;
      child->parent = parent;
    } else {
      // Node was root, set root
      tree->root = child;
      child->parent = nullptr;
    }

    child->red = false;
    delete node;
    clear_local_area_delete(child, flagged_nodes);
    return true;
  }

  // Node has no children
  // If Node is the root, just delete it
  if (node == tree->root) {
    tree->root = nullptr;
    delete node;
    // No need to clear local area bc tree is empty
    return true;
  }

  // If Node is red, just delete it
  if (node->red) {
    parent->child[parent->child[1] == node] = nullptr;
    delete node;
    clear_local_area_delete(parent, flagged_nodes);
    return true;
  }

  // Node is childless and black (Delete Node and Rebalance)
  int dir = (parent->child[1] == node);
  parent->child[dir] = nullptr;
  TreeNode tmp = node;
  node = nullptr;
  delete tmp;

  // Fix up by rebalancing the tree
  // Proprogate the deletion up the tree until reaching root

  TreeNode sibling, close_nephew, distant_nephew;
  while (node != tree->root) {
    dir = parent->child[1] == node;
    sibling = parent->child[1-dir];
    distant_nephew = sibling->child[1-dir];
    close_nephew = sibling->child[dir];
    
    // In cases D3, D6, D5, D4, the node is black
    if (sibling->red) { // Fixup cases 1 (red sibling) & 2 (sibling's children are black)
      // Case D3
      return delete_case_3(tree, node, parent, sibling, close_nephew, distant_nephew, dir, flagged_nodes);
    } else if (distant_nephew && distant_nephew->red) {
      // Case D6
      return delete_case_6(tree, parent, sibling, distant_nephew, dir, flagged_nodes);
    } else if (close_nephew && close_nephew->red) {
      // Case D5
      return delete_case_5(tree, node, parent, sibling, close_nephew, distant_nephew, dir, flagged_nodes);
    } else if (parent->red) {
      // Case D4
      return delete_case_4(sibling, parent, flagged_nodes);
    }
    // Else case - node is red
    sibling->red = true;
    node = parent;
    parent = node->parent;
    clear_local_area_delete(node, flagged_nodes);
  }
  return true;
}

TreeNode delete_fixup(TreeNode node, )

/*
get_markers_above()

1. setup local area
2. old_marker_nodes = get nodes with markers above curr node
3. perform rotation
4. mark the new set of 4 ancestor nodes with markers
5. iterate over old_marker_nodes, clear markers
6. clear local area

*/

// Runs parallel insert on values
void tree_delete_bulk(Tree &tree, vector<int> values, int batch_size, int num_threads) {
  int num_operations = values.size();

  #pragma omp parallel for schedule(static, batch_size) num_threads(num_threads)
  for (int i = 0; i < num_operations; i++) {
      tree_delete(tree, values[i]);
  }
}