// Test discovery of functions called from an instrumented region.
//
// Negative test: calls to a function that calls an unmarked function outside of
// its TU won't increase the instrumentation count

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int marked_call_unmarked_outside_tu(int);
int tm_marked_call_unmarked_outside_tu(int);
}

int main() {
  report<int>("callinst_004", "unsafe call to unmarked function in another TU",
              {{marked_call_unmarked_outside_tu(74),
                tm_marked_call_unmarked_outside_tu(74)}},
              {{TM_STATS_TRANSLATE_NOTFOUND, 1},
               {TM_STATS_UNSAFE, 1},
               {TM_STATS_LOAD_U4, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
int something = 5;
int unmarked_in_other_tu_callable(int i);
TX_SAFE int marked_call_unmarked_outside_tu(int i) {
  return unmarked_in_other_tu_callable(i);
}
}
#endif

#ifdef TEST_OFILE2
extern "C" {
extern int something;
int unmarked_in_other_tu_callable(int i) { return i * something * something; }
}
#endif