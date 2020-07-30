// Test of advanced C++ dispatch facility.
//
// Here we are testing calls of function pointers to marked functions from
// within a transaction

#ifdef TEST_DRIVER
#include "../include/harness.h"

int test_fptr(void (*f)());
void marked();

int main() {
  report<int>(
      "dispatch_005",
      "Calling a function pointer to marked function does not serialize",
      {{test_fptr(marked), 77}},
      {{TM_STATS_STORE_U4, 1},
       {TM_STATS_UNSAFE, 0},
       {TM_STATS_TRANSLATE_NOTFOUND, 0},
       {TM_STATS_TRANSLATE_FOUND, 1}});
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
TX_SAFE void marked() { x = 77; }
#endif