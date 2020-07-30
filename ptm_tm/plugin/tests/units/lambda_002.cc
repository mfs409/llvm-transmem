// Test of C++ lambdas for transaction boundaries
//
// Ensure that bodies of lambdas get instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"
int loadstore_lambda_test();
int main() {
  report<int>("lambda_002", "Lambda body loads/stores get instrumented",
              {{loadstore_lambda_test(), 55}}, {{TM_STATS_LOAD_U4, 1},{TM_STATS_STORE_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x;
int loadstore_lambda_test() {
  x = 55;
  int tmp = 0;
  TX_BEGIN { tmp = x; }
  TX_END;
  return tmp;
}
#endif

#ifdef TEST_OFILE2
#endif