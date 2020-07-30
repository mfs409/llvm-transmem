// Test of C++ raii for transaction boundaries
//
// Ensure that loop inside of transaction

#ifdef TEST_DRIVER
#include "../include/harness.h"
int loop_inside_raii_test();
int main() {
  report<int>("raii_008", "raii body loads/stores get instrumented",
              {{loop_inside_raii_test(), 50}},
              {{TM_STATS_LOAD_U4, 0}, {TM_STATS_STORE_U4, 50}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x;
int loop_inside_raii_test() {
  int tmp = 0;
  {
    TX_RAII;
    for (int i = 0; i < 50; i++) {
      tmp++;
      x = tmp;
    }
  }
  return x;
}
#endif

#ifdef TEST_OFILE2
#endif
