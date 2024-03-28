typedef struct TreeNode {
  int val;
  bool red;
  struct TreeNode* parent;
  struct TreeNode* left;
  struct TreeNode* right;
} *TreeNode;

typedef struct Tree {
  TreeNode root;
} *Tree;

inline TreeNode newTreeNode(int val, bool red, TreeNode parent, 
                            TreeNode left, TreeNode right) {
  TreeNode node;
  node->val = val;
  node->red = red;
  node->parent = parent;
  node->left = left;
  node->right = right;
  return node;
}

// Return Whether Red-Black Tree Rooted at root is valid
// If it is valid, also return the number of black nodes to any leaf,
// including this info allows validation to be written recursively
bool validateAtBlackDepth(TreeNode &root, int *blackDepth, int *lo, int *hi) {
  // (Base Case) Leaves are Valid
  if (!root) {
    *blackDepth = 0;
    return true;
  }

  // Root must follow BST invariant
  if (lo && root->val <= *lo || hi && *hi <= root->val) {
    return false;
  }

  // Red Nodes Cannot have Red Children
  TreeNode left = root->left, right = root->right;
  if (root->red && ((left && left->red) || (right && right->red))) {
    return false;
  }

  // Children Must Point back to their Parents
  if ((left && left->parent != root) || (right && right->parent != root)) {
    return false;
  }

  // Left and right subtrees must be valid red-black trees
  int *leftDepth, *rightDepth;
  bool leftValid = validateAtBlackDepth(root->left, leftDepth, lo, &(root->val));
  bool rightValid = validateAtBlackDepth(root->right, rightDepth, &(root->val), hi);
  if (!leftValid || !rightValid) {
    return false;
  }
  // Black depth must be the same for both children
  if (*leftDepth != *rightDepth) {
    return false;
  }
  *blackDepth = *leftDepth + !(root->red);
  return true;
}

// Return whether Red-Black Tree Rooted at root is valid
bool validate(Tree &tree) {
  int blackDepth = 0;
  return validateAtBlackDepth(tree->root, &blackDepth, nullptr, nullptr);
}

// Return whether a node with given value exists in a Red-Black Tree
bool lookup(Tree &tree, int val) {
  TreeNode node = tree->root;
  while (node) {
    if (val < node->val) {
      node = node->left;
    } else if (val > node->val) {
      node = node->right;
    } else {
      return true;
    }
  }
  return false;
}

// Inserts Node into Tree, returns True if Node Inserted (i.e. wasn't already present)
bool insert(Tree &tree, int val) {
  // Edge Case: Set root of empty tree
  if (!tree->root) {
    tree->root = newTreeNode(val, true, nullptr, nullptr, nullptr);
    return true;
  }

  // Search down to find where node would be
  TreeNode iter = tree->root;
  TreeNode parent = tree->root->parent;
  while (iter) {
    parent = iter;
    if (val < iter->val) {
      iter = iter->left;
    } else if (val > iter->val) {
      iter = iter->right;
    } else {
      return false;
    }
  }
  
}

int main() {
  return 0;
}