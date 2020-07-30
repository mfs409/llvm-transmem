// Test of load instrumentation.
//
// Here we ensure that a load of a double results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
double test_load_d();
double tm_test_load_d();
}

int main() {
  report<double>("load_007", "loads of doubles get instrumented",
                 {{test_load_d(), tm_test_load_d()}}, {{TM_STATS_LOAD_D, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern double var_d;
extern "C" {
TX_SAFE double test_load_d() {
  return var_d == 9.7884637261 ? 9.7884637261 : 0;
}
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
double var_d = 9.7884637261;
#endif
