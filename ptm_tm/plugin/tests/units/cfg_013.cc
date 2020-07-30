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
int varargs_accessed_test();
int main() {
  report<int>("cfg_013", "varargs accessed in raii get instrumented",
              {{varargs_accessed_test(), 3}},
              {{TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 0},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_STORE_U4, 2},
               {TM_STATS_LOAD_U4, 2}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cstdarg>
int a;
void simple_parse(const char *fmt...) {
  va_list args;
  va_start(args, fmt);

  while (*fmt != '\0') {
    if (*fmt == 'd') {
      a = va_arg(args, int);
    }
    ++fmt;
  }

  va_end(args);
}

int varargs_accessed_test() {
#define _THREE_
  {
    TX_RAII;
#ifdef _THREE_
    simple_parse("d", 3);
#else
    simple_parse("d", 4);
#endif
  }
  return a;
}
#endif

#ifdef TEST_OFILE2
#endif
