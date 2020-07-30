// Test of std::atomic relaxed load instrumentation.
//
// Here we ensure that an atomic relaxed load of a bool results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
bool test_arload_bool();
bool tm_test_arload_bool();
}

int main() {
  report<bool>("arload_001", "relaxed loads of atomic bools get instrumented",
               {{test_arload_bool(), tm_test_arload_bool()}},
               {{TM_STATS_UNSAFE, 1}, {TM_STATS_LOAD_U1, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<bool> var_bool;
extern "C" {
    TX_SAFE bool test_arload_bool() { return var_bool.load(std::memory_order_relaxed) == true ? true : false; }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<bool> var_bool = {true};
#endif
