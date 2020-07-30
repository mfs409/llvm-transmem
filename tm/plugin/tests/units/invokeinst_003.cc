// Test discovery of functions invoked from an instrumented region.
//
// Ensure that a chain of unmarked functions that are reached via invoke, and
// are in the same TU, get instrumented

struct exception {
  char tmp;
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
int something = 0;
extern "C" {
int marked_invoke_unmarked_chain_in_tu();
int tm_marked_invoke_unmarked_chain_in_tu();
}

int main() {
  report<int>("invoke_003", "chain of unmarked function invokes",
              {{marked_invoke_unmarked_chain_in_tu(),
                tm_marked_invoke_unmarked_chain_in_tu()}},
              {{TM_STATS_LOAD_U4, 1}, {TM_STATS_UNSAFE, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
// NB: we use noinline a lot, to keep clang from over-optimizing
extern int something;
extern "C" {
__attribute__((noinline)) int level3_invokable() {
  try {
    return something + something + something;
  } catch (exception any) {
    return -1;
  }
}
__attribute__((noinline)) int level2_invokable() {
  try {
    return level3_invokable();
  } catch (exception any) {
    return -1;
  }
}
__attribute__((noinline)) int level1_invokable() {
  try {
    return level2_invokable();
  } catch (exception any) {
    return -1;
  }
}
TX_SAFE int marked_invoke_unmarked_chain_in_tu() {
  try {
    return level1_invokable();
  } catch (exception any) {
    return -1;
  }
}
}
#endif

#ifdef TEST_OFILE2
#endif