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
  int num_operations, num_threads, val;
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
  }

  // Attempt to open and read file
  cout << "Input file: " << input_filename << '\n';
  ifstream fin(input_filename);

  if (!fin) {
    cerr << "Unable to open file: " << input_filename << ".\n";
    exit(EXIT_FAILURE);
  }

  string line;
  // Keep reading until the end of the file
  while (getline(cin, line)) {
    stringstream ss(line);
    string operation;
    int operation_type = atoi(operation.c_str());
    ss >> operation;

    switch (operation_type) {
      case INSERT:
        ss >> num_operations;
        operations.push_back({vector<int>(num_operations), INSERT});
        break;
      case DELETE:
        ss >> num_operations;
        operations.push_back({vector<int>(num_operations), DELETE});
        break;
      case LOOKUP:
        ss >> num_operations;
        operations.push_back({vector<int>(num_operations), LOOKUP});
        break;
      default:
        vector<int> values = operations.back().values;
        // values.resize(num_operations);

        for (int i = 0; i < num_operations; i++) {
          ss >> val;
          values[i] = val;
        }
    }
  }

  printf("Printing inputs:\n");
  for (auto& operation : operations) {
    printf("Operation %d\n", operation.type);
    printf("Values:\n");
    for (auto& val : operation.values) {
      printf("%d ", val);
    }
    printf("\n");
  }

  // TODO: Add *correct* code for random input fuzzing test later
  // mixed_test


  // Start Red-Black Testing Code Here
  // int expected_size = 0;
  // Tree tree = tree_init();
  // for (auto& operation : operations) {
  //   printf("%d, %d\n", operation.type, operation.val);
  //   switch(operation.type) {
  //     case INSERT:
  //       if (tree_insert(tree, operation.val)) {
  //         expected_size++;
  //       }
  //       break;
  //     case DELETE:
  //       if (tree_delete(tree, operation.val)) {
  //         expected_size--;
  //       }
  //       break;
  //     case LOOKUP:
  //       tree_lookup(tree, operation.val);
  //       break;
  //   }
  //   if (!tree_validate(tree)) {
  //     cout << "Produced invalid Tree at operation " << operation_to_string(operation) << ".\n";
  //     return 1;
  //   }
  //   else if (tree_size(tree) != expected_size) {
  //     cout << "Produced Tree of wrong size at operation " << operation_to_string(operation) << ".\n";
  //     return 1;
  //   }
  // }
  printf("Success.\n");
  return 0;
}