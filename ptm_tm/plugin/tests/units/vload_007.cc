// Test of volatile load instrumentation.
//
// Here we ensure that a load of a volatile double results in a proper call to
// the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
double test_vload_d();
double tm_test_vload_d();
}

int main() {
  report<double>("vload_007", "loads of volatile doubles get instrumented",
                 {{test_vload_d(), tm_test_vload_d()}},
                 {{TM_STATS_LOAD_D, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile double var_d;
extern "C" {
TX_SAFE double test_vload_d() {
  return var_d == 9.7884637261 ? 9.7884637261 : 0;
}
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile double var_d = {9.7884637261};
#endif
