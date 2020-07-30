// Test of C++ raii for transaction boundaries
//
// throw from a raii transaction get instrumented

// need tests of atomics
// need tests of invoke
// need loop inside of transaction
// need transaction inside of loop
// need return from transaction
// need throw from transaction
// need break from loop from transaction
// need varargs accessed in transaction
// check if landing pad for a transaction gets instrumented
// maybe copy all invoke, all throwcatch, atomic, volatiles

#ifdef TEST_DRIVER
#include "../include/harness.h"
int throw_from_transaction_test();
int main() {
  report<int>("cfg_011", "throw from raii get instrumented",
              {{throw_from_transaction_test(), 0}},
              {{TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 1},
               {TM_STATS_UNSAFE, 4},
               {TM_STATS_STORE_U4, 0},
               {TM_STATS_LOAD_U4, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <iostream>
__attribute__((noinline)) TX_SAFE double division(int a, int b) {
  if (b == 0) {
    throw "Division by zero condition!";
  }
  return (a / b);
}
int throw_from_transaction_test() {
  {
    double z = 0;
    {
      TX_RAII;
      int x = 50;
      int y = 0;

      try {
        z = division(x, y);
      } catch (const char *msg) {
        printf("%s", msg);
      }
    }

    return 0;
  }
}
#endif

#ifdef TEST_OFILE2
#endif
