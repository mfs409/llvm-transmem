// Test of volatile load instrumentation.
//
// Here we ensure that a volatile load of a float results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
float test_vload_f();
float tm_test_vload_f();
}

int main() {
  report<float>("vload_006", "loads of volatile floats get instrumented",
                {{test_vload_f(), tm_test_vload_f()}},
                {{TM_STATS_LOAD_F, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile float var_f;
extern "C" {
TX_SAFE float test_vload_f() { return var_f == 1.5f ? 1.5f : 0; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile float var_f = {1.5f};
#endif
