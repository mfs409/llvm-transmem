// Test of C++ raii for transaction boundaries
//
// break from loop in raii get instrumented

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
int break_from_transaction_test();
int main() {
  report<int>("cfg_012", "break from loop in raii get instrumented",
              {{break_from_transaction_test(), 50}},
              {{TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 0},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_STORE_U4, 50},
               {TM_STATS_LOAD_U4, 50}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <iostream>
int x = 0;
int break_from_transaction_test() {
  {
    TX_RAII;
    for (int i = 0; i < 50; i++) {
      x++;
      if (x == 50)
        break;
    }
  }
  return x;
}
#endif

#ifdef TEST_OFILE2
#endif
