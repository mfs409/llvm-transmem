// Test of volatile load instrumentation.
//
// Here we ensure that a volatile load of a bool results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
bool test_vload_bool();
bool tm_test_vload_bool();
}

int main() {
  report<bool>("vload_001", "loads of volatile bools get instrumented",
               {{test_vload_bool(), tm_test_vload_bool()}},
               {{TM_STATS_UNSAFE, 1}, {TM_STATS_LOAD_U1, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile bool var_bool;
extern "C" {
TX_SAFE bool test_vload_bool() { return var_bool == true ? true : false; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile bool var_bool = {true};
#endif
