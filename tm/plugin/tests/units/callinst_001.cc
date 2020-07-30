// Test discovery of functions called from an instrumented region.
//
// Make sure that when an unmarked function is reachable from a marked function,
// it gets instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int unmarked_in_tu_callable(int);
int tm_unmarked_in_tu_callable(int);
}

int main() {
  report<int>("callinst_001", "marked function calls unmarked function",
              {{unmarked_in_tu_callable(88), tm_unmarked_in_tu_callable(88)}},
              {{TM_STATS_LOAD_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"

extern "C" {
int something = 5;

// not marked, but called by marked function, so expect it to be instrumented
int unmarked_in_tu_callable(int i) { return i * something; }

TX_SAFE int marked_call_unmarked_in_tu(int i) {
  return unmarked_in_tu_callable(i);
}
}
#endif

#ifdef TEST_OFILE2
#endif