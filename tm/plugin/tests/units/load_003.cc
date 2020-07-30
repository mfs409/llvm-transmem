// Test of load instrumentation.
//
// Here we ensure that a load of a short results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
short test_load_u2();
short tm_test_load_u2();
}

int main() {
  report<short>("load_003", "loads of shorts get instrumented",
                {{test_load_u2(), tm_test_load_u2()}}, {{TM_STATS_LOAD_U2, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern short var_u2;
extern "C" {
TX_SAFE short test_load_u2() { return var_u2 == (short)0xE8E8 ? 0xE8E8 : 0; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
short var_u2 = 0xE8E8;
#endif
