// Test of volatile load instrumentation.
//
// Here we ensure that a volatile load of a long int results in a proper call to
// the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
long test_vload_u8();
long tm_test_vload_u8();
}

int main() {
  report<long>("vload_005", "loads of volatile longs get instrumented",
               {{test_vload_u8(), tm_test_vload_u8()}},
               {{TM_STATS_LOAD_U8, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile unsigned long var_u8;
extern "C" {
TX_SAFE long test_vload_u8() {
  return var_u8 == 0xC6C6C6C6C6C6C6C6L ? 0xC6C6C6C6C6C6C6C6L : 0;
}
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile unsigned long var_u8 = {0xC6C6C6C6C6C6C6C6L};
#endif
