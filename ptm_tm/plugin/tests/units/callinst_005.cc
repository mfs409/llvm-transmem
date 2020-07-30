// Test discovery of functions called from an instrumented region.
//
// make sure that when a marked function calls a marked function, the clone gets
// called

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int marked_call_marked(int);
int tm_marked_call_marked(int);
}

int main() {
  report<int>("callinst_005", "marked function calls marked function",
              {{marked_call_marked(99), tm_marked_call_marked(99)}},
              {{TM_STATS_LOAD_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
int something = 5;
__attribute__((noinline)) TX_SAFE int marked_func_callable(int i) {
  return i + something;
}
TX_SAFE int marked_call_marked(int i) { return marked_func_callable(i); }
}
#endif

#ifdef TEST_OFILE2
#endif