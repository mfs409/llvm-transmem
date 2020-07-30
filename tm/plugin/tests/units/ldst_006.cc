// Test of load and store instrumentation.
//
// Here we ensure that a load and then store of a float results in a proper call
// to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_loadstore_f(float);
void tm_test_loadstore_f(float);
}

extern float var_f;

int main() {
    mem_test<float, float>("ldst_006",
                  "loads followed by stores of floats get instrumented",
                  {test_loadstore_f, tm_test_loadstore_f}, &var_f, 12, 0xCA,
                  {{TM_STATS_STORE_F, 1}, {TM_STATS_LOAD_F, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern float var_f;
extern "C" {
TX_SAFE void test_loadstore_f(float v) { var_f = var_f + v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
float var_f = 12;
#endif
