#include <atomic>
#include <vector>
#include <string>
#include <omp.h>
#include <stdlib.h>

using namespace std;

enum OperationType {
  INSERT,
  DELETE,
  LOOKUP,
  EMPTY
};

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
void tree_insert_bulk(Tree &tree, vector<int> values, int batch_size, int num_threads);
void tree_delete_bulk(Tree &tree, vector<int> values, int batch_size, int num_threads);

// (Sequential) Debug Functions
int tree_size(Tree &tree);
bool tree_validate(Tree &tree);
string tree_to_string(Tree tree);
vector<int> tree_to_vector(Tree &tree);

// Lock-free Debug functions
void tree_to_vec(TreeNode &node, vector<int> &vec, vector<int> &flags);
void print_tree(TreeNode &node);

// Helper Functions for Lock-free Operations
void clear_local_area_insert(TreeNode &node);
void clear_local_area_delete(TreeNode &node);
void is_in_local_area(TreeNode &node);
void move_local_area_up(TreeNode &node);

// Make sure to check the setup succeeded
bool setup_local_area_insert(TreeNode &node);
bool setup_local_area_delete(TreeNode &node);

typedef struct Operation {
  vector<int> values;
  int type;
} Operation_t;

// Helper functions
string operation_to_string(Operation_t operation);