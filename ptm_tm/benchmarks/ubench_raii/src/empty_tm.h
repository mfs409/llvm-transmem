#pragma once

#include <functional>

#include "../../../common/tm_api.h"

#include "../common/config.h"

/// empty_tm_cpp is used to test the overhead of just calling TXN block in C++.
template <typename T> class empty_tm_cpp {
  __attribute__((noinline)) void execute_empty_tm() {
    { TX_RAII; }
  }

public:
  empty_tm_cpp(config *cfg) {}

  bool contains(T val) {
    execute_empty_tm();
    return true;
  }

  bool remove(T val) {
    execute_empty_tm();
    return true;
  }

  bool insert(T val) {
    execute_empty_tm();
    return true;
  }

  void check() {}
};

/// empty_tm_c is used to test the overhead of just calling TXN block in C.
template <typename T> class empty_tm_c {
  static void test(void *) {}

  __attribute__((noinline)) void execute_empty_tm() {
    TX_BEGIN_C(nullptr, test, NULL);
  }

public:
  empty_tm_c(config *cfg) {}

  bool contains(T val) {
    execute_empty_tm();
    return true;
  }

  bool remove(T val) {
    execute_empty_tm();
    return true;
  }

  bool insert(T val) {
    execute_empty_tm();
    return true;
  }

  void check() {}
};

/// empty is the class we do nothing, it is
/// used as a comparison with empty_tm
template <typename T> class empty_non {
  __attribute__((noinline)) void execute_empty() {}

public:
  empty_non(config *cfg) {}

  bool contains(T val) {
    execute_empty();
    return true;
  }

  bool remove(T val) {
    execute_empty();
    return true;
  }

  bool insert(T val) {
    execute_empty();
    return true;
  }

  void check() {}
};
