// Test discovery of functions called from an instrumented region.
//
// Using RENAME to substitute safe functions for library functions should work
// across TUs

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int marked_call_special(int);
int tm_marked_call_special(int);
}

int main() {
  report<int>("callinst_007",
              "Substitute safe alternative to unmarked function across TUs",
              {{marked_call_special(7), 44}, {tm_marked_call_special(7), 2}},
              {{TM_STATS_TRANSLATE_FOUND, 1},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_LOAD_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
int something = 5;
int subtract(int i);
int tm_special_function(int);
TX_SAFE int marked_call_special(int i) { return subtract(i); }
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
extern "C" {
extern int something;
int subtract(int i) { return i * i - something; }
__attribute__((noinline)) TX_RENAME(subtract) int special_function(int i) {
  return i - something;
}
}
#endif