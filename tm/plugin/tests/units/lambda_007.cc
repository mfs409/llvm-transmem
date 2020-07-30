// Test of C++ lambdas for transaction boundaries
//
// Across-TU nesting test for lambda-based transactions

#ifdef TEST_DRIVER
#include "../include/harness.h"

int nested_call_in_tu_test();
int main() {
  report<int>("lambda_007", "lambda nesting across TUs",
              {{nested_call_in_tu_test(), 120}},
              {{TM_STATS_TRANSLATE_FOUND, 1},
               {TM_STATS_TRANSLATE_NOTFOUND, 0},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_STORE_U4, 2}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x = 0;
void nested_call_in_tu();
int nested_call_in_tu_test() {
  x = 77;
  TX_BEGIN { nested_call_in_tu(); }
  TX_END;
  return x;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
extern int x;
TX_SAFE void nested_call_in_tu() {
  x = x + 37;
  TX_BEGIN { x = x + 6; }
  TX_END;
}
#endif