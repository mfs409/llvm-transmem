// Test of std::atomic relaxed  store instrumentation.
//
// Here we ensure that a relaxed store of an atomic long results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_arstore_u8(long);
void tm_test_arstore_u8(long);
}

extern std::atomic<long> var_u8;

int main() {
    mem_test<long, long>("arstore_005", "relaxed stores of atomic longs get instrumented",
      {test_arstore_u8, tm_test_arstore_u8}, (long*)&var_u8, 0,
      0xCAFEBEEFBABEBABEL, {{TM_STATS_STORE_U8, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<long> var_u8;
extern "C" {
    TX_SAFE void test_arstore_u8(long v) { var_u8.store(v, std::memory_order_relaxed); }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<long> var_u8 = {0};
#endif
