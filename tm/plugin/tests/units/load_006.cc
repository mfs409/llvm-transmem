// Test of load instrumentation.
//
// Here we ensure that a load of a float results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
float test_load_f();
float tm_test_load_f();
}

int main() {
  report<float>("load_006", "loads of floats get instrumented",
                {{test_load_f(), tm_test_load_f()}}, {{TM_STATS_LOAD_F, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern float var_f;
extern "C" {
TX_SAFE float test_load_f() { return var_f == 1.5f ? 1.5f : 0; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
float var_f = 1.5f;
#endif
