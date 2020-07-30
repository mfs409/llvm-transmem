// Test of std::atomic relaxed  store instrumentation.
//
// Here we ensure that a relaxed store of an atomic int results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_arstore_u4(int);
void tm_test_arstore_u4(int);
}

extern std::atomic<int> var_u4;

int main() {
    mem_test<int, int>("arstore_004", "relaxed stores of atomic ints get instrumented",
      {test_arstore_u4, tm_test_arstore_u4}, (int*)&var_u4, 0, 0xCAFEBEEF,
      {{TM_STATS_STORE_U4, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<int> var_u4;
extern "C" {
    TX_SAFE void test_arstore_u4(int v) { var_u4.store(v, std::memory_order_relaxed); }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<int> var_u4 = {0};
#endif
