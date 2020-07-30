// Test of store instrumentation.
//
// Here we ensure that a store of a long double results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_store_ld(long double);
void tm_test_store_ld(long double);
}

extern long double var_ld;

int main() {
    mem_test<long double, long double>("store_008", "stores of long doubles get instrumented",
                        {test_store_ld, tm_test_store_ld}, &var_ld, 0, 1.999L,
                        {{TM_STATS_STORE_LD, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern long double var_ld;
extern "C" {
TX_SAFE void test_store_ld(long double v) { var_ld = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
long double var_ld = 0;
#endif
