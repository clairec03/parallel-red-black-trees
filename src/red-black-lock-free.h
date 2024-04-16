#include <atomic>
#include <vector>
#include <string>

#include <omp.h>

using namespace std;

typedef struct RedBlackNode {
  struct RedBlackNode* child[2];
  struct RedBlackNode* parent;
  int val;
  int marker;
  atomic<bool> flag;
  bool red;
} *TreeNode;

typedef struct RedBlackTree {
  TreeNode root;
} *Tree;

// Tree Functions
Tree tree_init();
bool tree_insert(Tree &tree, int val);
bool tree_delete(Tree &tree, int val);
bool tree_lookup(Tree &tree, int val);

// (Sequential) Debug Functions
int tree_size(Tree &tree);
bool tree_validate(Tree &tree);
string tree_to_string(Tree tree);
vector <int> tree_to_vector(Tree &tree);