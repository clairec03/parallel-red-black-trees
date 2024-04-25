#include "red-black-sequential.h"

std::string operation_to_string(struct Operation operation) {
  switch (operation.type) {
    case INSERT:
      return "INSERT " + std::to_string(operation.val);
      break;
    case DELETE:
      return "DELETE " + std::to_string(operation.val);
      break;
    case LOOKUP:
      return "LOOKUP " + std::to_string(operation.val);
      break;
    default:
      return "INVALID OPERATION";
  }
}