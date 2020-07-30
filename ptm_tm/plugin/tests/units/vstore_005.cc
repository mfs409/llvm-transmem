// Test of volatile store instrumentation.
//
// Here we ensure that a store of a volatile long results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_vstore_u8(long);
void tm_test_vstore_u8(long);
}

extern volatile long var_u8;

int main() {
    mem_test<long, long>("vstore_005", "stores of volatile longs get instrumented",
      {test_vstore_u8, tm_test_vstore_u8}, (long*)&var_u8, 0,
      0xCAFEBEEFBABEBABEL, {{TM_STATS_STORE_U8, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile long var_u8;
extern "C" {
TX_SAFE void test_vstore_u8(long v) { var_u8 = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile long var_u8 = {0};
#endif
