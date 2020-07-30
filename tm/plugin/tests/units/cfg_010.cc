// Test of C++ raii for transaction boundaries
//
// return from transaction(raii) get instrumented

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
int return_from_transaction_test();
int main() {
  report<int>("raii_011", "return from raii get instrumented",
              {{return_from_transaction_test(), 10}},
              {{TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 0},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_STORE_U4, 10},
               {TM_STATS_LOAD_U4, 10}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x = 0;
int return_from_transaction_test() {
  int y = 0;
  {
    TX_RAII;
    for (; y < 100; y++) {
      x++;
      if (y == 9) {
        return x;
      }
    }
  }
  return x;
}
#endif

#ifdef TEST_OFILE2
#endif
