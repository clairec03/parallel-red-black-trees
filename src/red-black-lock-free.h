#include <atomic>
#include <vector>
#include <set>
#include <string>

using namespace std;

typedef struct RedBlackNode {
  struct RedBlackNode* parent;
  struct RedBlackNode* child[2];
  int val;
  int marker;
  bool red;
  atomic<bool> locked;
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
string tree_to_string(Tree tree);
vector <int> tree_to_vector(Tree &tree);