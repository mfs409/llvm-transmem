// Test of volatile store instrumentation.
//
// Here we ensure that a store of a volatile long double results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_vstore_ld(long double);
void tm_test_vstore_ld(long double);
}

extern volatile long double var_ld;

int main() {
    mem_test<long double, long double>("vstore_008", "stores of volatile long doubles get instrumented",
      {test_vstore_ld, tm_test_vstore_ld}, (long double*)&var_ld, 0, 1.999L,
      {{TM_STATS_STORE_LD, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile long double var_ld;
extern "C" {
TX_SAFE void test_vstore_ld(long double v) { var_ld = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile long double var_ld = {0};
#endif
