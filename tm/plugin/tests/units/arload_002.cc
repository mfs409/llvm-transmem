// Test of std::atomic relaxed load instrumentation.
//
// Here we ensure that an atomic relaxed load of a char (u1) results in a proper call to
// the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
char test_arload_u1();
char tm_test_arload_u1();
}

int main() {
  report<char>("arload_002", "relaxed loads of atomic chars get instrumented",
               {{test_arload_u1(), tm_test_arload_u1()}},
               {{TM_STATS_LOAD_U1, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<unsigned char> var_u1;
extern "C" {
TX_SAFE char test_arload_u1() {
  return var_u1.load(std::memory_order_relaxed) == (unsigned char)0xF9 ? 0xF9 : 0;
}
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<unsigned char> var_u1 = {0xF9};
#endif
