// Test of C++ raii for transaction boundaries
//
// varargs accessed in raii get instrumented

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
int loop_in_loop_test();
int main() {
  report<int>("cfg_013", "varargs accessed in raii get instrumented",
              {{loop_in_loop_test(), 25}},
              {{TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 0},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_STORE_U4, 25},
               {TM_STATS_LOAD_U4, 25}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cstdarg>
int value = 0;
int loop_in_loop_test() {

  for (int i = 0; i < 5; i++) {
    {
      TX_RAII;
      for (int k = 0; k < 5; k++)
        value++;
    }
  }
  return value;
}
#endif

#ifdef TEST_OFILE2
#endif
