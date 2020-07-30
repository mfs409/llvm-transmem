// Test of load and store instrumentation.
//
// Here we ensure that a load and then store of a long results in a proper call
// to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_loadstore_u8(long);
void tm_test_loadstore_u8(long);
}

extern long var_u8;

int main() {
    mem_test<long, long>("ldst_005",
                 "loads followed by stores of longs get instrumented",
                 {test_loadstore_u8, tm_test_loadstore_u8}, &var_u8, 11, 0xCA,
                 {{TM_STATS_STORE_U8, 1}, {TM_STATS_LOAD_U8, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern long var_u8;
extern "C" {
TX_SAFE void test_loadstore_u8(long v) { var_u8 += v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
long var_u8 = 10;
#endif
