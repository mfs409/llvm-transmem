// Test of C++ raii for transaction boundaries
//
// Ensure that transaction inside of loop

#ifdef TEST_DRIVER
#include "../include/harness.h"
int raii_inside_loop_test();
int main() {
  report<int>("raii_009", "raii transaction inside loop get instrumented",
              {{raii_inside_loop_test(), 50}},
              {{TM_STATS_LOAD_U4, 0}, {TM_STATS_STORE_U4, 50}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x;
int raii_inside_loop_test() {
  int tmp = 0;
  for (int i = 0; i < 50; i++) {
    TX_RAII;
    tmp++;
    x = tmp;
  }

  return x;
}
#endif

#ifdef TEST_OFILE2
#endif
