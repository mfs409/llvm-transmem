// Test of store instrumentation.
//
// Here we ensure that a store of an int results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_store_u4(int);
void tm_test_store_u4(int);
}

extern int var_u4;

int main() {
    mem_test<int, int>("store_004", "stores of ints get instrumented",
                {test_store_u4, tm_test_store_u4}, &var_u4, 0, 0xCAFEBEEF,
                {{TM_STATS_STORE_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern int var_u4;
extern "C" {
TX_SAFE void test_store_u4(int v) { var_u4 = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
int var_u4 = 0;
#endif
