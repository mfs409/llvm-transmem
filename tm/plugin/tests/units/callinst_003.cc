// Test discovery of functions called from an instrumented region.
//
// Calling a pure function should not produce an instrumented body

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int marked_call_pure_function_in_tu(int);
int tm_marked_call_pure_function_in_tu(int);
}

int main() {
  report<int>("callinst_003", "pure functions do not get instrumented",
              {{marked_call_pure_function_in_tu(37),
                tm_marked_call_pure_function_in_tu(37)}},
              {{TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_LOAD_U4, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {

int something = 5;

TX_PURE int pure_function(int i) { return something; }

TX_SAFE int marked_call_pure_function_in_tu(int i) { return pure_function(i); }
}
#endif

#ifdef TEST_OFILE2
#endif