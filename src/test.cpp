#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <unistd.h>
#include <vector>

using namespace std;

int main()  {
    while ((opt = getopt(argc, argv, "f:")) != -1) {
    switch (opt) {
      case 'f':
        input_filename = optarg;
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
    std::cout << "Input file: " << input_filename << '\n';
    std::ifstream fin(input_filename);

    if (!fin) {
      std::cerr << "Unable to open file: " << input_filename << ".\n";
      exit(EXIT_FAILURE);
    }

    fin >> num_operations;
    fin >> operation_type;

    std::vector<int> operations;
    operations.resize(num_operations);
    int val;
    for (int operation : operations) {
      fin >> operation.val;
    }
  } else if (insert_test) {
    operations.resize(num_operations);
    for (auto& operation : operations) {
      operation.type = INSERT;
      operation.val = std::rand();
    }
  } else { // mixed_test
    operations.resize(num_operations);
    std::vector<int> in_tree;
    int index;
    for (auto& operation : operations) {
      if (in_tree.size() > 0) {
        operation.type = std::rand() % 3;
      } else {
        operation.type = INSERT;
      }
      
      switch (operation.type) {
        case INSERT:
          operation.val = std::rand();
          in_tree.push_back(operation.val);
          break;
        case DELETE:
          index = std::rand() % in_tree.size();
          operation.val = in_tree[index];
          in_tree.erase(in_tree.begin() + index);
          break;
        case LOOKUP:
          index = std::rand() % in_tree.size();
          operation.val = in_tree[index];
          break;
      }
    }
  }
}