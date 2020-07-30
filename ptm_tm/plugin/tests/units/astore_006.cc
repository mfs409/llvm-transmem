// Test of std::atomic store instrumentation.
//
// Here we ensure that a store of an atomic float results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_astore_f(float);
void tm_test_astore_f(float);
}

extern std::atomic<float> var_f;

int main() {
    mem_test<float, float>("astore_006", "stores of atomic floats get instrumented",
      {test_astore_f, tm_test_astore_f}, (float*)&var_f, 0, 1.5f,
      {{TM_STATS_STORE_F, 1}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<float> var_f;
extern "C" {
TX_SAFE void test_astore_f(float v) { var_f = v; }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<float> var_f = {0};
#endif
