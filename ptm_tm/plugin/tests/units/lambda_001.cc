// Test of C++ lambdas for transaction boundaries
//
// This is the most basic test.  It just ensures that we get boundary
// instrumentation for lambdas

#ifdef TEST_DRIVER
#include "../include/harness.h"
int base_lambda_test();
int main() {
  report<int>("lambda_001", "Transaction via lambda gets boundary instrumentation",
              {{base_lambda_test(), 77}}, {{TM_STATS_BEGIN_OUTER, 1},
              {TM_STATS_END_OUTER, 1},
              {TM_STATS_UNSAFE, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x;
int base_lambda_test() {
  x = 0;
  TX_BEGIN { x = 77; }
  TX_END;
  return x;
}
#endif

#ifdef TEST_OFILE2
#endif