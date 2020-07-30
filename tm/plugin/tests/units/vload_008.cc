// Test of volatile load instrumentation.
//
// Here we ensure that a load of a volatile long double results in a proper call
// to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
long double test_vload_ld();
long double tm_test_vload_ld();
}

int main() {
  report<long double>("vload_008",
                      "loads of volatile long doubles get instrumented",
                      {{test_vload_ld(), tm_test_vload_ld()}},
                      {{TM_STATS_LOAD_LD, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile long double var_ld;
extern "C" {
TX_SAFE long double test_vload_ld() {
  return var_ld == 99.7777564534251638495L ? 99.7777564534251638495L : 0;
}
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile long double var_ld{99.7777564534251638495L};
#endif
