// Test of std::atomic load instrumentation.
//
// Here we ensure that an atomic load of a long int results in a proper call to
// the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
long test_aload_u8();
long tm_test_aload_u8();
}

int main() {
  report<long>("aload_005", "loads of atomic longs get instrumented",
               {{test_aload_u8(), tm_test_aload_u8()}},
               {{TM_STATS_LOAD_U8, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<unsigned long> var_u8;
extern "C" {
TX_SAFE long test_aload_u8() {
  return var_u8 == 0xC6C6C6C6C6C6C6C6L ? 0xC6C6C6C6C6C6C6C6L : 0;
}
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<unsigned long> var_u8 = {0xC6C6C6C6C6C6C6C6L};
#endif
