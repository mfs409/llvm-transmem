#pragma once

#include <functional>

#include "../../../common/tm_api.h"

#include "../common/config.h"

/// counter_tm is an array of shared counters.  It uses coarse-grained
/// transactions for concurrency control
template <typename T> class counter_tm {

  int *counters;
  const size_t num_counters;

  void increment_counter(int which) {
    {
      TX_RAII;
      counters[which]++;
    }
  }

public:
  /// Construct an array of counters
  counter_tm(config *cfg) : num_counters(cfg->key_range) {
    counters = new int[num_counters];
    for (size_t i = 0; i < num_counters; ++i) {
      counters[i] = 0;
    }
  }

  /// contains is coerced into an increment
  bool contains(T val) {
    increment_counter(val);
    return true;
  }

  /// remove is coerced into an increment
  bool remove(T val) {
    increment_counter(val);
    return true;
  }

  /// insert is coerced into an increment
  bool insert(T val) {
    increment_counter(val);
    return true;
  }

  void check() {
    using std::cout;
    using std::endl;
    for (size_t i = 0; i < num_counters; ++i)
      cout << counters[i] << ", ";
    cout << endl;
  }
};