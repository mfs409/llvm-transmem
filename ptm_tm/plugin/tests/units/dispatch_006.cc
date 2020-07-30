// Test of advanced C++ dispatch facility.
//
// Here we are testing calls of function pointers to unmarked functions from
// within a transaction

#ifdef TEST_DRIVER
#include "../include/harness.h"

int test_fptr(void (*f)());
void unmarked();

int main() {
  report<int>("dispatch_006",
              "Calling a function pointer to unmarked function serializes",
              {{test_fptr(unmarked), 88}},
              {{TM_STATS_STORE_U4, 0},
               {TM_STATS_UNSAFE, 1},
               {TM_STATS_TRANSLATE_NOTFOUND, 1},
               {TM_STATS_TRANSLATE_FOUND, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int x = 0;
int test_fptr(void (*f)()) {
  TX_BEGIN { f(); }
  TX_END;
  return x;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
extern int x;
void unmarked() { x = 88; }
#endif