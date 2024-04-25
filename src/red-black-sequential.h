#include <vector>
#include <string>

using namespace std;

#define INSERT 0
#define DELETE 1
#define LOOKUP 2

typedef struct RedBlackNode {
  struct RedBlackNode* child[2];
  struct RedBlackNode* parent;
  int val;
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

// Debug Functions
int tree_size(Tree &tree);
bool tree_validate(Tree &tree);
string tree_to_string(Tree tree);
vector <int> tree_to_vector(Tree &tree);

typedef struct Operation {
    int type;
    int val; 
} Operation_t;

// Helper functions
string operation_to_string(Operation_t operation);