// Test of std::atomic relaxed  store instrumentation.
//
// Here we ensure that a relaxed store of an atomic float results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_arstore_f(float);
void tm_test_arstore_f(float);
}

extern std::atomic<float> var_f;

int main() {
    mem_test<float, float>("arstore_006", "relaxed stores of atomic floats get instrumented",
      {test_arstore_f, tm_test_arstore_f}, (float*)&var_f, 0, 1.5f,
      {{TM_STATS_STORE_F, 1}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<float> var_f;
extern "C" {
    TX_SAFE void test_arstore_f(float v) { var_f.store(v, std::memory_order_relaxed); }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<float> var_f = {0};
#endif
