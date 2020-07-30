// Test of volatile store instrumentation.
//
// Here we ensure that a store of a volatile float results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_vstore_f(float);
void tm_test_vstore_f(float);
}

extern volatile float var_f;

int main() {
    mem_test<float, float>("vstore_006", "stores of volatile floats get instrumented",
      {test_vstore_f, tm_test_vstore_f}, (float*)&var_f, 0, 1.5f,
      {{TM_STATS_STORE_F, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile float var_f;
extern "C" {
TX_SAFE void test_vstore_f(float v) { var_f = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile float var_f = {0};
#endif
