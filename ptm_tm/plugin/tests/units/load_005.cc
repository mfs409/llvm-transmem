// Test of load instrumentation.
//
// Here we ensure that a load of a long int results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
long test_load_u8();
long tm_test_load_u8();
}

int main() {
  report<long>("load_005", "loads of longs get instrumented",
               {{test_load_u8(), tm_test_load_u8()}}, {{TM_STATS_LOAD_U8, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern long var_u8;
extern "C" {
TX_SAFE long test_load_u8() {
  return var_u8 == 0xC6C6C6C6C6C6C6C6L ? 0xC6C6C6C6C6C6C6C6L : 0;
}
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
long var_u8 = 0xC6C6C6C6C6C6C6C6L;
#endif
