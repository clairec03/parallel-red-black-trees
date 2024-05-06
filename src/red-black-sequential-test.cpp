#include <iostream>
#include <fstream>
#include <string>
#include <set>

#include <unistd.h>

#include "red-black-sequential.h"

using namespace std;

string operation_to_string(Operation operation) {
  switch(operation.type){
    case INSERT:
      return "INSERT " + operation.val;
    case DELETE:
      return "DELETE " + operation.val;
    case LOOKUP:
      return "LOOKUP " + operation.val;
    default:
      return "(INVALID)";
  }
}

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

int main(int argc, char *argv[]) {
  // Command Line Input Code (adapted from Assn 3)
  int opt;
  bool insert_test = false, mixed_test = false;
  int num_operations = 0;
  while ((opt = getopt(argc, argv, "i:m:")) != -1) {
    switch (opt) {
      case 'i':
        insert_test = true;
        num_operations = atoi(optarg);
        break;
      case 'm':
        mixed_test = true;
        num_operations = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Usage: %s -i / -m \n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  // Should only specify one of i, m
  if (insert_test + mixed_test != 1) {
    fprintf(stderr, "Usage: %s -i / -m \n", argv[0]);
    exit(EXIT_FAILURE);
  }

  vector<Operation_t> operations;
  if (insert_test) {
    operations.resize(num_operations);
    for (auto& operation : operations) {
      operation.type = INSERT;
      operation.val = rand();
    }
  }
  else { // mixed_test
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
  }

  // Start Red-Black Testing Code Here
  int expected_size = 0;
  Tree tree = tree_init();
  for (auto& operation : operations) {
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