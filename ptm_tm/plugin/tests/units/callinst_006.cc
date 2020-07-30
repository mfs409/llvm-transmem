// Test discovery of functions called from an instrumented region.
//
// Calls to a function that calls a marked function outside of its TU should
// work

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int marked_call_marked_outside_tu(int);
int tm_marked_call_marked_outside_tu(int);
}

int main() {
  report<int>("callinst_006", "safe call to unmarked function in another TU",
              {{marked_call_marked_outside_tu(54),
                tm_marked_call_marked_outside_tu(54)}},
              {{TM_STATS_TRANSLATE_FOUND, 1}, {TM_STATS_UNSAFE, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
int something = 5;
TX_SAFE int marked_in_other_tu_callable(int i);
TX_SAFE int marked_call_marked_outside_tu(int i) {
  return marked_in_other_tu_callable(i);
}
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
extern "C" {
extern int something;
TX_SAFE int marked_in_other_tu_callable(int i) {
  return i * something + something;
}
}
#endif