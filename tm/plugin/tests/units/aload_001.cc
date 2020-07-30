// Test of std::atomic load instrumentation.
//
// Here we ensure that an atomic load of a bool results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
bool test_aload_bool();
bool tm_test_aload_bool();
}

int main() {
  report<bool>("aload_001", "loads of atomic bools get instrumented",
               {{test_aload_bool(), tm_test_aload_bool()}},
               {{TM_STATS_UNSAFE, 1}, {TM_STATS_LOAD_U1, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<bool> var_bool;
extern "C" {
TX_SAFE bool test_aload_bool() { return var_bool == true ? true : false; }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<bool> var_bool = {true};
#endif
