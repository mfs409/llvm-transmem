// Test of std::atomic store instrumentation.
//
// Here we ensure that a store of an atomic short results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_astore_u2(short);
void tm_test_astore_u2(short);
}

extern std::atomic<short> var_u2;

int main() {
    mem_test<short, short>("astore_003", "stores of atomic shorts get instrumented",
      {test_astore_u2, tm_test_astore_u2}, (short*)&var_u2, 0, 0xCAFE,
      {{TM_STATS_STORE_U2, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<short> var_u2;
extern "C" {
TX_SAFE void test_astore_u2(short v) { var_u2 = v; }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<short> var_u2 = {0};
#endif
