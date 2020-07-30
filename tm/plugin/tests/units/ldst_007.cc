// Test of load and store instrumentation.
//
// Here we ensure that a load and then store of a double results in a proper
// call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_loadstore_d(double);
void tm_test_loadstore_d(double);
}

extern double var_d;

int main() {
    mem_test<double, double>("ldst_007",
                   "loads followed by stores of doubles get instrumented",
                   {test_loadstore_d, tm_test_loadstore_d}, &var_d, 13, 0xCA,
                   {{TM_STATS_STORE_D, 1}, {TM_STATS_LOAD_D, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern double var_d;
extern "C" {
TX_SAFE void test_loadstore_d(double v) { var_d = var_d + v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
double var_d = 13;
#endif
