#pragma once

#include <functional>

#include "../../../common/tm_api.h"

#include "../common/config.h"

#include "list_tm.h"

/// hash_tm is fixed-size bucket array of singly-linked, sorted lists.  It uses
/// coarse-grained transactions for concurrency control
template <typename T> class hash_tm {

  list_tm<T> **buckets;
  const size_t num_buckets;
  std::hash<T> internal_hash;

public:
  /// Construct a hashtable by creating an array of lists
  hash_tm(config *cfg) : num_buckets(cfg->buckets) {
    buckets = new list_tm<T> *[num_buckets]();
    for (size_t i = 0; i < num_buckets; ++i) {
      buckets[i] = new list_tm<T>(cfg);
    }
  }

  /// Search for a key in the hash
  bool contains(T val) {
    size_t idx = internal_hash(val) % num_buckets;
    return buckets[idx]->contains(val);
  }

  /// Remove an element from the hash
  bool remove(T val) {
    size_t idx = internal_hash(val) % num_buckets;
    return buckets[idx]->remove(val);
  }

  /// Insert a value into the hash
  bool insert(T val) {
    size_t idx = internal_hash(val) % num_buckets;
    return buckets[idx]->insert(val);
  }

  void check() {
    // TODO
  }
};