// Test of load instrumentation.
//
// Here we ensure that a load of a long double results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
long double test_load_ld();
long double tm_test_load_ld();
}

int main() {
  report<long double>("load_008", "loads of long doubles get instrumented",
                      {{test_load_ld(), tm_test_load_ld()}},
                      {{TM_STATS_LOAD_LD, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern long double var_ld;
extern "C" {
TX_SAFE long double test_load_ld() {
  return var_ld == 99.7777564534251638495L ? 99.7777564534251638495L : 0;
}
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
long double var_ld = 99.7777564534251638495L;
#endif
