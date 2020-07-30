// Test of C++ lambdas for transaction boundaries
//
// Ensure that same-TU unmarked calls from a lambda get instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"
int unmarked_call_lambda_test();
int main() {
  report<int>("lambda_004", "Unmarked in-TU calls from lambda get instrumented",
              {{unmarked_call_lambda_test(), 100}},
              {{TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 0},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_STORE_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x = 0;
__attribute__((noinline)) void unmarked(int val) { x = val * val; }
int unmarked_call_lambda_test() {
  x = 66;
  TX_BEGIN { unmarked(10); }
  TX_END;
  return x;
}
#endif

#ifdef TEST_OFILE2
#endif