#include <vector>
#include <set>
#include <string>

typedef struct RedBlackNode {
  int val;
  bool red;
  struct RedBlackNode* parent;
  struct RedBlackNode* child[2];
} *TreeNode;

typedef struct RedBlackTree {
  TreeNode root;
} *Tree;

// Tree Functions
Tree tree_init();
bool tree_insert(Tree &tree, int val);
bool tree_delete(Tree &tree, int val);
bool tree_lookup(Tree &tree, int val);

// Debug Functions
int tree_size(Tree &tree);
bool tree_validate(Tree &tree);
std::string tree_to_string(Tree tree);
std::vector <int> tree_to_vector(Tree &tree);