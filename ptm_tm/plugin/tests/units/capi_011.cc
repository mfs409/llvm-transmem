// Test execution of transactions via the C API
//
// Here we are looking at what happens when we cannot statically find the clone
// that TX_BEGIN_C should call, but a valid clone does exist

#ifdef TEST_DRIVER
#include "../include/harness.h"

int call_indirect();
int main() {
  report<int>("capi_011",
              "Start transaction from indirect pointer to (marked) function in "
              "other TU",
              {{call_indirect(), 99297}},
              {{TM_STATS_UNSAFE, 0},
               {TM_STATS_TRANSLATE_FOUND, 1},
               {TM_STATS_LOAD_U4, 1},
               {TM_STATS_STORE_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern int val;
extern "C" {
TX_SAFE void indirect_marked_callable(void *) { val += 99297; }
}
void (*func)(void *) = indirect_marked_callable;
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
extern void (*func)(void *);
int val = 0;
int call_indirect() {
  TX_BEGIN_C(nullptr, func, nullptr);
  return val;
}
#endif