#include "red-black-lock-free.h"
#include <stdlib.h>

/* Helper functions for lock-free */
void clear_local_area_insert(TreeNode node) {
  // Release all flags in the local area
  TreeNode p = NULL, gp = NULL, u = NULL;

  // First, reset all flags to false
  if (node) {
    node->flag = false;
    p = node->parent;
  }
  if (p) {
    p->flag = false;
    gp = p->parent;
  }
  if (gp) {
    u = gp->child[gp->child[1] != p];
    if (u) u->flag = false;
  }

  // Then, clear all markers on up to 4 ancestors (including gp) and reset to -1
  TreeNode marker_node = gp;
  for (int i = 0; i < 4; i++) {
    if (marker_node) {
      marker_node->marker = -1;
      marker_node = marker_node->parent;
    }
  }
}


void clear_local_area_delete(TreeNode node) {}
void is_local_area_available(TreeNode node) {}

/* Helper functions for insert */
bool setup_local_area_insert(TreeNode node) {
  TreeNode p, gp, u;
  p = node->parent;
  if (p) gp = p->parent;
  if (gp) u = gp->child[gp->child[1] != p];

  // Setup Flags
  bool expected = false;
  if (!p->flag.compare_exchange_weak(expected, true)) {
    return false;
  }
  if (!gp->flag.compare_exchange_weak(expected, true)) {
    p->flag = false;
    return false;
  }
  if (!u->flag.compare_exchange_weak(expected, true)) {
    p->flag = false;
    gp->flag = false;
    return false;
  }

  int thread_id = omp_get_thread_num();

  // Setup Markers
  TreeNode marker_node = gp;
  for (int i = 0; i < 4; i++) {
    if (marker_node) {
      if (marker_node->marker == -1) {
        marker_node->marker = thread_id;
        marker_node = marker_node->parent;
      } else {
        // Writing to the i-th marker failed
        // Reset all flags
        p->flag = false;
        gp->flag = false;   
        u->flag = false;
        
        // Reset all markers up to the (i-1)-th marker (inclusive)
        marker_node = gp;
        for (int j = 0; j < i - 1; j++) {
          if (marker_node) {
            marker_node->marker = -1;
            marker_node = marker_node->parent;
          }
        }

        return false;
      }
    }
  }  
  return true;
}

/* Helper functions for delete */
bool setup_local_area_delete() {}