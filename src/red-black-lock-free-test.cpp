#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include "red-black-lock-free.h"
#include <omp.h>
#include <vector>
#include <set>
#include <sstream>
#include <unordered_map>

using namespace std;

int main(int argc, char *argv[]) {
  // Command Line Input Code (adapted from Lab 3)
  string input_filename;
  int opt;
  bool insert_test = false;
  int num_operations, val;
  int num_threads = 1;
  int batch_size = 8;
  bool correctness = false;
  vector<Operation_t> operations;

  while ((opt = getopt(argc, argv, "f:b:n:c")) != -1) {
    switch (opt) {
      case 'f':
        input_filename = optarg;
        break;
      case 'b':
        batch_size = atoi(optarg);
        break;
      case 'n':
        num_threads = atoi(optarg);
        break;
      case 'c':
        correctness = true;
      default:
        fprintf(stderr, "Usage: %s -f input_filename -n num_threads -b batch_size\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (empty(input_filename) || batch_size <= 0 || num_threads < 1) {
    fprintf(stderr, "Usage: %s -f input_filename -n num_threads -b batch_size\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Attempt to open and read file
  cout << "Input file: " << input_filename << '\n';
  ifstream fin(input_filename);

  if (!fin) {
    cerr << "Unable to open file: " << input_filename << ".\n";
    exit(EXIT_FAILURE);
  }

  unordered_map<string, int> operation_map = {
    {"INSERT", 0},
    {"DELETE", 1},
    {"LOOKUP", 2}
  };

  string line;
  // Keep reading until the end of the file
  while (getline(fin, line)) {
    stringstream ss(line);
    string operation;
    ss >> operation;
    int operation_type;

    if (operation_map.find(operation) != operation_map.end()) {
      operation_type = operation_map[operation];
    } else {
      operation_type = EMPTY;
    }

    switch (operation_type) {
      case INSERT:
        ss >> num_operations;
        operations.push_back({{}, INSERT});
        break;
      case DELETE:
        ss >> num_operations;
        operations.push_back({{}, DELETE});
        break;
      case LOOKUP:
        ss >> num_operations;
        operations.push_back({{}, LOOKUP});
        break;
      default:
        vector<int> values = vector<int>(num_operations);
        val = stoi(operation);
        for (int i = 0; i < num_operations; i++) {
          values[i] = val;
          ss >> val;
        }
        operations.back().values = values;
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

  // Now for the actual testing!
  Tree tree = tree_init();
  set<int> correct_values;
  for (Operation_t operation : operations) {
    if (operation.type == INSERT) {
      tree_insert_bulk(tree, operation.values, batch_size, num_threads);
      if (correctness) {
        for (auto value : operation.values) {
          correct_values.insert(value);
        }
      }
    } else if (operation.type == DELETE) {
      tree_delete_bulk(tree, operation.values, batch_size, num_threads);
      if (correctness) {
        for (auto value : operation.values) {
          correct_values.erase(value);
        }
      }
    }

    if (correctness) {
      if (!tree_validate(tree)) {
        printf("Testing failed.\n");
        exit(1);
      }
      // Ensure tree has correct elems
      vector<int> tree_values = tree_to_vector(tree);
      if (tree_values.size() != correct_values.size()) {
        printf("Tree has incorrect size\n");
        printf("Testing failed\n");
        exit(1);
      }
      for (int i = 0; i < tree_values.size(); i++) {
        if (correct_values.find(tree_values[i]) == correct_values.end()) {
          printf("Tree contains value not present in correct code\n");
          printf("Testing failed\n");
          exit(1);
        }
      }
    }
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