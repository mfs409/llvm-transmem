// Test of C++ lambdas for transaction boundaries
//
// This is the most basic test.  It just ensures that we get boundary
// instrumentation for raii

#ifdef TEST_DRIVER
#include "../include/harness.h"
extern "C" {
int base_cfg_test();
}
int main() {
  report<int>("cfg_001", "Transaction via TX gets boundary instrumentation",
              {{base_cfg_test(), 77}},
              {{TM_STATS_BEGIN_OUTER, 1},
               {TM_STATS_END_OUTER, 1},
               {TM_STATS_UNSAFE, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
int x;
int base_cfg_test() {
  x = 0;
  {
    TX_RAII;
    x = 77;
  }
  return x;
}
}
#endif

#ifdef TEST_OFILE2
#endif
