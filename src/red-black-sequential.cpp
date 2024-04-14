#include "red-black-sequential.h"

inline TreeNode newTreeNode(int val, bool red, TreeNode parent, 
                            TreeNode left, TreeNode right) {
  TreeNode node = new struct RedBlackNode();
  node->val = val;
  node->red = red;
  node->parent = parent;
  node->child[0] = left;
  node->child[1] = right;
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



std::string subtreeToString(TreeNode root) {
  if (!root) {
    return "Empty";
  }
  if (root->red) 
    return "RED(" + subtreeToString(root->child[0]) + ", " + std::to_string(root->val) + ", " + subtreeToString(root->child[1]) + ")";
  else 
    return "BLACK(" + subtreeToString(root->child[0]) + ", " + std::to_string(root->val) + ", " + subtreeToString(root->child[1]) + ")";
}

std::string tree_to_string(Tree T) {
  return subtreeToString(T->root);
}

int size_subtree(TreeNode &root) {
  if (!root) return 0;
  return 1 + size_subtree(root->child[0]) + size_subtree(root->child[1]);
}

void inord_tree_to_vec_helper(TreeNode T, std::vector <int> &res) {
  if (!T) return;
  inord_tree_to_vec_helper(T->child[0], res);
  res.push_back(T->val);
  inord_tree_to_vec_helper(T->child[1], res);
}

std::vector <int> tree_to_vector(Tree &T) {
  TreeNode root = T->root;
  std::vector <int> res;
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
  int blackDepth = 0;
  return validateAtBlackDepth(tree->root, &blackDepth, nullptr, nullptr);
}

// Return whether a node with given value exists in a Red-Black Tree
bool tree_lookup(Tree &tree, int val) {
  TreeNode node = tree->root;
  while (node) {
    if (val < node->val) {
      node = node->child[0];
    } else if (val > node->val) {
      node = node->child[1];
    } else {
      return true;
    }
  }
  return false;
}

// Inserts Node into Tree, returns True if Node Inserted (i.e. wasn't already present)
bool tree_insert(Tree &tree, int val) {
  // Edge Case: Set root of Empty tree
  if (!tree->root) {
    printf("HERE\n");
    tree->root = newTreeNode(val, true, nullptr, nullptr, nullptr);
    return true;
  }

  // Search down to find where node would be
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
  if (val < parent->val) {
    parent->child[0] = node;
  } else {
    parent->child[1] = node;
  }

  // Go Through the Cases of Tree Insertion
  // Source: https://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Insertion
  TreeNode grandparent;
  TreeNode uncle;
  int dir;
  while (node->parent) {
    // If Parent is Black, Chilling (I1)
    if (!parent->red) {
      return true;
    }

    // If Parent is Red Root, Turn Black and Return (I4)
    grandparent = parent->parent;
    if (!grandparent) {
      parent->red = false;
      return true;
    }

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

      rotateDir(tree, grandparent, 1-dir);
      parent->red = false;
      grandparent->red = true;
      return true;
    }

    // Parent and Uncle Both Red, Swap Parent + Grandparent Colors (I2)
    parent->red = false;
    uncle->red = false;
    grandparent->red = true;

    node = grandparent;
    parent = node->parent;
  }

  // If We're the Root, Done (I3)
  return true;
}

// DELETE HELPER FUNCTIONS (As per Wikipedia)
bool delete_case_6(Tree &tree, TreeNode parent, TreeNode sibling, TreeNode distant_nephew, int dir) {
  rotateDir(tree, parent, dir);
  sibling->red = parent->red;
  parent->red = false;
  distant_nephew->red = false;
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
  close_nephew = sibling->child[dir]; // close   nephew
  if (close_nephew && close_nephew->red)
    return delete_case_5(tree, parent, sibling, close_nephew, distant_nephew, dir);
  return delete_case_4(sibling, parent);
}

bool tree_delete (Tree &tree, int val) {
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
      node = iter;
      break;
    } else {
      // Delete from left child if true, right child if false
      parent = iter;
      iter = iter->child[(val > iter->val)];
    }
  }
  // If we never found the node to delete, don't delete it
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

  // One Node Case
  if (child) {
    // Replace Node with its extant child
    if (parent) {
      // Node had parent, set parent's child
      parent->child[parent->child[1] == node] = child;
      child->parent = node->parent;
    }
    else {
      // Node was root, set root
      tree->root = child;
    }
    child->red = false;
    delete node;
    return true;
  }

  // Node has no children
  // If Node is the root, just delete it
  if (node == tree->root) {
    tree->root = nullptr;
    delete node;
    return true;
  }

  // If Node is red, just delete it
  if (node->red) {
    parent->child[parent->child[1] == node] = nullptr;
    delete node;
    return true;
  }

  // Node is childless and black (Delete Node and Rebalance)
  int dir = parent->child[1] == node;
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
    }
    else if (distant_nephew && distant_nephew->red) {
      // Case D6
      return delete_case_6(tree, parent, sibling, distant_nephew, dir);
    }
    else if (close_nephew && close_nephew->red) {
      // Case D5
      return delete_case_5(tree, parent, sibling, close_nephew, distant_nephew, dir);
    }
    else if (parent->red) {
      // Case D4
      return delete_case_4(sibling, parent);
    }
    sibling->red = true;
    node = parent;
    parent = node->parent;
  }
  return true;

}