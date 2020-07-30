// Test of volatile store instrumentation.
//
// Here we ensure that a store of a volatile double results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_vstore_d(double);
void tm_test_vstore_d(double);
}

extern volatile double var_d;

int main() {
    mem_test<double, double>("vstore_007", "stores of volatile doubles get instrumented",
      {test_vstore_d, tm_test_vstore_d}, (double*)&var_d, 0, 1.77,
      {{TM_STATS_STORE_D, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile double var_d;
extern "C" {
TX_SAFE void test_vstore_d(double v) { var_d = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile double var_d = {0};
#endif
