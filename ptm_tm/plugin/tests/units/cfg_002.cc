// Test of C++ cfg for transaction boundaries
//
// Ensure that bodies of cfg get instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"
int loadstore_cfg_test();
int main() {
  report<int>("cfg_002", "CFG body loads/stores get instrumented",
              {{loadstore_cfg_test(), 55}},
              {{TM_STATS_LOAD_U4, 1}, {TM_STATS_STORE_U4, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x;
int loadstore_cfg_test() {
  x = 55;
  int tmp = 0;
  {
    TX_RAII;
    tmp = x;
  }
  return tmp;
}
#endif

#ifdef TEST_OFILE2
#endif
