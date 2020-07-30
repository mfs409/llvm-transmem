// Test discovery of functions called from an instrumented region.
//
// Make sure that when we have a chain of unmarked calls that are reachable from
// a marked function, all get instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int marked_call_unmarked_chain_in_tu();
int tm_marked_call_unmarked_chain_in_tu();
}

int main() {
  report<int>("callinst_002", "chain of unmarked function calls in TU",
              {{marked_call_unmarked_chain_in_tu(),
                tm_marked_call_unmarked_chain_in_tu()}},
              {{TM_STATS_LOAD_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"

extern "C" {

int something = 5;

// NB: need to use noinline, to prevent -O3 overoptimizing the tests

__attribute__((noinline)) int level3_callable() {
  return something + something + something;
}

__attribute__((noinline)) int level2_callable() { return level3_callable(); }

__attribute__((noinline)) int level1_callable() { return level2_callable(); }

// The clone of this function should wind up calling the clone of level3
TX_SAFE int marked_call_unmarked_chain_in_tu() { return level1_callable(); }
}
#endif

#ifdef TEST_OFILE2
#endif