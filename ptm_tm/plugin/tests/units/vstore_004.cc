// Test of volatile store instrumentation.
//
// Here we ensure that a store of a volatile int results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_vstore_u4(int);
void tm_test_vstore_u4(int);
}

extern volatile int var_u4;

int main() {
    mem_test<int, int>("vstore_004", "stores of volatile ints get instrumented",
      {test_vstore_u4, tm_test_vstore_u4}, (int*)&var_u4, 0, 0xCAFEBEEF,
      {{TM_STATS_STORE_U4, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile int var_u4;
extern "C" {
TX_SAFE void test_vstore_u4(int v) { var_u4 = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile int var_u4 = {0};
#endif
