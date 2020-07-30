#pragma once

#include <functional>

#include "../../../common/tm_api.h"

#include "../common/config.h"

/// list_tm is a singly-linked, sorted list that uses coarse-grained
/// transactions for concurrency control
template <typename T> class list_tm {
  /// node_t is the base type for nodes in the list
  struct node_t {
    T val;
    node_t *next;
  };

  /// Head pointer for the singly-linked list
  node_t *head;

public:
  /// Construct a list by creating a sentinel node at the head
  list_tm(config *cfg) : head(new node_t()) {}

  /// Search for a key in the singly-linked list, starting from the head
  bool contains(T val) {
    bool res = false;
    TX_BEGIN {
      node_t *curr = head->next;
      while (curr != nullptr && curr->val < val) {
        curr = curr->next;
      }
      res = (curr != nullptr && curr->val == val);
    }
    TX_END;
    return res;
  }

  /// Remove an element from the list
  bool remove(T val) {
    bool res = false;
    TX_BEGIN {
      node_t *prev = head;
      node_t *curr = prev->next;
      while (curr != nullptr && curr->val < val) {
        prev = curr;
        curr = curr->next;
      }
      // On match, unlink
      if (curr != nullptr && curr->val == val) {
        prev->next = curr->next;
        free(curr);
        res = true;
      }
      // TODO: we assume that res gets reset... gotta check
    }
    TX_END;
    return res;
  }

  /// Insert a value into the list.
  bool insert(T val) {
    bool res = false;
    TX_BEGIN {
      node_t *prev = head;
      node_t *curr = prev->next;
      while (curr != nullptr && curr->val < val) {
        prev = curr;
        curr = curr->next;
      }
      if (curr == nullptr || curr->val != val) {
        // No match: insert node
        node_t *to_insert = (node_t *)malloc(sizeof(node_t));
        to_insert->val = val;
        to_insert->next = curr;
        prev->next = to_insert;
        res = true;
      }
    }
    TX_END;
    return res;
  }

  void check() {
    // TODO
  }
};