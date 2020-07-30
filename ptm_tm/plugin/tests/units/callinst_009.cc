// Test discovery of functions called from an instrumented region.
//
// When we call a pure function from the root of a C++ lambda, it should
// not get instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
bool tx_begin_call_pure();
}

extern bool var_bool;

int main() {
  report<bool>("callinst_009", "lambdas do not instrument pure functions",
               {{tx_begin_call_pure(), true}}, {{TM_STATS_STORE_U1, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
__attribute__((noinline)) TX_PURE void pure_function(bool *target, bool value) {
  *target = value;
}

bool tx_begin_call_pure() {
  bool ret = false;
  TX_BEGIN { pure_function(&ret, true); }
  TX_END;
  return ret;
}
}
#endif

#ifdef TEST_OFILE2
#endif
