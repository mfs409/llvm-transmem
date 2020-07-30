// Test of std::atomic relaxed load instrumentation.
//
// Here we ensure that an atomic relaxed load of a short results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
short test_arload_u2();
short tm_test_arload_u2();
}

int main() {
  report<short>("arload_003", "relaxed loads of atomic shorts get instrumented",
                {{test_arload_u2(), tm_test_arload_u2()}},
                {{TM_STATS_LOAD_U2, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<unsigned short> var_u2;
extern "C" {
TX_SAFE short test_arload_u2() {
  return var_u2.load(std::memory_order_relaxed) == (unsigned short)0xE8E8 ? 0xE8E8 : 0;
}
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<unsigned short> var_u2 = {0xE8E8};
#endif
