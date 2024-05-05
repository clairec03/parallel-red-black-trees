#include <atomic>
#include <vector>
#include <string>
#include <omp.h>
#include <stdlib.h>

using namespace std;

#define DEFAULT_MARKER -1

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
  atomic<bool> root_flag; // Used to ensure only one node sets root value
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
void clear_local_area_insert(TreeNode &node, vector<TreeNode> &flagged_nodes);
void clear_local_area_delete(TreeNode &node, vector<TreeNode> &flagged_nodes);
void move_local_area_up_insert(TreeNode &node, vector<TreeNode> &flagged_nodes);
void move_local_area_up_delete(TreeNode &node, vector<TreeNode> &flagged_nodes);
bool get_markers_and_flags_above_delete(TreeNode &node);
// bool get_markers_above_delete(TreeNode &start, bool release);
// bool get_flags_above_delete();

// Make sure to check the setup succeeded
bool setup_local_area_insert(TreeNode &node);
bool setup_local_area_delete(TreeNode &successor, TreeNode &node, vector<TreeNode> &flagged_nodes);

typedef struct Operation {
  vector<int> values;
  int type;
} Operation_t;

// Helper functions
string operation_to_string(Operation_t operation);

// Parallel tree operations
void tree_insert_bulk(Tree &tree, vector<int> values, int batch_size, int num_threads);
void tree_delete_bulk(Tree &tree, vector<int> values, int batch_size, int num_threads);