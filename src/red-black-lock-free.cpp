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
    move_local_area_up(node); // TODO: Correctness check
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
bool delete_case_6(Tree &tree, TreeNode parent, TreeNode sibling, TreeNode distant_nephew, int dir) {
  rotateDir(tree, parent, dir);
  sibling->red = parent->red;
  parent->red = false;
  distant_nephew->red = false;
  clear_local_area_delete(parent);
  return true;
}

bool delete_case_5(Tree &tree, TreeNode parent, TreeNode sibling, 
                   TreeNode close_nephew, TreeNode distant_nephew, int dir) {
  rotateDir(tree, sibling, 1-dir);
  sibling->red = true;
  close_nephew->red = false;
  distant_nephew = sibling;
  sibling = close_nephew;
  return delete_case_6(tree, parent, sibling, distant_nephew, dir);
}

bool delete_case_4(TreeNode &sibling, TreeNode &parent) {
  sibling->red = true;
  parent->red = false;
  clear_local_area_delete(parent);
  return true;
}

bool delete_case_3(Tree &tree, TreeNode parent, TreeNode sibling, 
                   TreeNode close_nephew, TreeNode distant_nephew, int dir) {
  rotateDir(tree, parent, dir);
  parent->red = true;
  sibling->red = false;
  sibling = close_nephew;
  // now: P red && S black
  distant_nephew = sibling->child[1-dir];
  if (distant_nephew && distant_nephew->red)
    return delete_case_6(tree, parent, sibling, distant_nephew, dir);
  close_nephew = sibling->child[dir]; // close nephew
  if (close_nephew && close_nephew->red)
    return delete_case_5(tree, parent, sibling, close_nephew, distant_nephew, dir);
  return delete_case_4(sibling, parent);
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
    clear_local_area_delete(child);
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
    clear_local_area_delete(parent);
    return true;
  }

  // Node is childless and black (Delete Node and Rebalance)
  int dir = (parent->child[1] == node);
  parent->child[dir] = nullptr;
  TreeNode tmp = node;
  node = nullptr;
  delete tmp;

  TreeNode sibling, close_nephew, distant_nephew;
  while (node != tree->root) {
    dir = parent->child[1] == node;
    sibling = parent->child[1-dir];
    distant_nephew = sibling->child[1-dir];
    close_nephew = sibling->child[dir];
    if (sibling->red) {
      // Case D3
      return delete_case_3(tree, parent, sibling, close_nephew, distant_nephew, dir);
    } else if (distant_nephew && distant_nephew->red) {
      // Case D6
      return delete_case_6(tree, parent, sibling, distant_nephew, dir);
    } else if (close_nephew && close_nephew->red) {
      // Case D5
      return delete_case_5(tree, parent, sibling, close_nephew, distant_nephew, dir);
    } else if (parent->red) {
      // Case D4
      return delete_case_4(sibling, parent);
    }
    sibling->red = true;
    node = parent;
    parent = node->parent;
    clear_local_area_delete(node);
  }
  return true;
}

// Runs parallel insert on values
void tree_delete_bulk(Tree &tree, vector<int> values, int batch_size, int num_threads) {
  int num_operations = values.size();

  #pragma omp parallel for schedule(static, batch_size) num_threads(num_threads)
  for (int i = 0; i < num_operations; i++) {
      tree_delete(tree, values[i]);
  }
}