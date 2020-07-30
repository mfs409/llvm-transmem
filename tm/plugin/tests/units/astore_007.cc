// Test of std::atomic store instrumentation.
//
// Here we ensure that a store of an atomic double results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_astore_d(double);
void tm_test_astore_d(double);
}

extern std::atomic<double> var_d;

int main() {
    mem_test<double, double>("astore_007", "stores of atomic doubles get instrumented",
      {test_astore_d, tm_test_astore_d}, (double*)&var_d, 0, 1.77,
      {{TM_STATS_STORE_D, 1}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<double> var_d;
extern "C" {
TX_SAFE void test_astore_d(double v) { var_d = v; }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<double> var_d = {0};
#endif
