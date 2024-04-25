#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <unistd.h>
#include "red-black-lock-free.h"
#include <omp.h>
#include <vector>
#include <sstream>

using namespace std;

vector<int> generate_rand_input(int length = 1000) {
  vector<int> res;
  set<int> seen; 
  int new_elem;
  for (int i = 0; i < length; i++) {
    new_elem = rand();
    
    while (seen.find(new_elem) != seen.end()) {
      new_elem = rand();
    }

    res.push_back(new_elem);
    seen.insert(new_elem);

  }
  return res;
}

int run_tests() {
  Tree tree = tree_init();
  int num_nodes = 10000;
  vector<int> tree_elems = generate_rand_input(num_nodes);

  for (int i = 0; i < num_nodes; i++) {
    tree_insert(tree, tree_elems[i]);
    if (!tree_validate(tree)) {
      return 1;
    }
    if (tree_size(tree) != i+1) {
      return 1;
    }
  }

  vector<int> vec_repr = tree_to_vector(tree);

  if (vec_repr.size() != tree_elems.size()) {
    printf("SIZE MISMATCH!\n");
    return 1;
  }

  for (int i = 1; i < num_nodes; i++) {
    if (vec_repr[i-1] >= vec_repr[i]) {
      printf("ELEMENT %d NOT IN ORDER!\n", i+1);
      return 1;
    }
  }
  
  for (int i = 0; i < num_nodes; i++) {
    tree_delete(tree, tree_elems[i]);
    if (!tree_validate(tree)) {
      printf("FAILED AT i = %d!\n", i);
      return 1;
    }
    if (tree_size(tree) != num_nodes-i-1) {
      printf("ELEMENT %d NOT ACTUALLY DELETED!\n", i);
      return 1;
    }
  }

  printf("SUCCEEDED!\n");
  return 0;
}

int main(int argc, char *argv[]) {
  // Command Line Input Code (adapted from Lab 3)
  string input_filename;
  int opt;
  bool insert_test = false;
  int num_operations, max_num_elems, num_threads;
  string operation_type;
  vector<Operation_t> operations;

  while ((opt = getopt(argc, argv, "f:n:")) != -1) {
    switch (opt) {
      case 'f':
        input_filename = optarg;
        break;
      case 'n':
        num_threads = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Usage: %s -f input_filename \n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (empty(input_filename)) {
    fprintf(stderr, "Usage: %s -f input_filename \n", argv[0]);
    exit(EXIT_FAILURE);
  } else {
    // Attempt to open and read file
    cout << "Input file: " << input_filename << '\n';
    ifstream fin(input_filename);

    if (!fin) {
      cerr << "Unable to open file: " << input_filename << ".\n";
      exit(EXIT_FAILURE);
    }

    fin >> num_operations;
    fin >> max_num_elems;

    if (max_num_elems <= 0) {
      cerr << "Invalid max_num_elems: " << max_num_elems << ".\n";
      exit(EXIT_FAILURE);
    }
    

    operations.resize(num_operations);
    int val;
    for (auto& operation : operations) {
      fin >> operation.val;
    }
  }

  for (int i = 0; i < num_operations; i++) {

  }
  // TODO: Add *correct* code for random input fuzzing test later
  // mixed_test
  operations.resize(num_operations);
  vector<int> in_tree;
  int index;
  for (auto& operation : operations) {
    if (in_tree.size() > 0) {
      operation.type = rand() % 3;
    } else {
      operation.type = INSERT;
    }
    
    switch (operation.type) {
      case INSERT:
        operation.val = rand();
        in_tree.push_back(operation.val);
        break;
      case DELETE:
        index = rand() % in_tree.size();
        operation.val = in_tree[index];
        in_tree.erase(in_tree.begin() + index);
        break;
      case LOOKUP:
        index = rand() % in_tree.size();
        operation.val = in_tree[index];
        break;
    }
  }

  // Start Red-Black Testing Code Here
  int expected_size = 0;
  Tree tree = tree_init();
  for (auto& operation : operations) {
    printf("%d, %d\n", operation.type, operation.val);
    switch(operation.type) {
      case INSERT:
        if (tree_insert(tree, operation.val)) {
          expected_size++;
        }
        break;
      case DELETE:
        if (tree_delete(tree, operation.val)) {
          expected_size--;
        }
        break;
      case LOOKUP:
        tree_lookup(tree, operation.val);
        break;
    }
    if (!tree_validate(tree)) {
      cout << "Produced invalid Tree at operation " << operation_to_string(operation) << ".\n";
      return 1;
    }
    else if (tree_size(tree) != expected_size) {
      cout << "Produced Tree of wrong size at operation " << operation_to_string(operation) << ".\n";
      return 1;
    }
  }
  printf("Success.\n");
  return 0;
}