// Test of C++ lambdas for transaction boundaries
//
// Ensure that calls within a lambda get instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"
int marked_call_lambda_test();
int main() {
  report<int>("lambda_003", "Lambda body calls get instrumented",
              {{marked_call_lambda_test(), 77}},
              {{TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 0},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_STORE_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x = 0;
__attribute__((noinline)) TX_SAFE void setx(int val) { x = val; }
int marked_call_lambda_test() {
  x = 66;
  TX_BEGIN { setx(77); }
  TX_END;
  return x;
}
#endif

#ifdef TEST_OFILE2
#endif