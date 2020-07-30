// Test execution of transactions via the C API
//
// Here we are looking at what happens when we cannot statically find the clone
// that TX_BEGIN_C should call, and a valid clone does not exist

#ifdef TEST_DRIVER
#include "../include/harness.h"

int call_indirect();
int main() {
  report<int>("capi_012",
              "Start transaction from indirect pointer to (unmarked) function "
              "in other TU",
              {{call_indirect(), 99297}},
              {{TM_STATS_UNSAFE, 1},
               {TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 1},
               {TM_STATS_LOAD_U4, 0},
               {TM_STATS_STORE_U4, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern int val;
extern "C" {
void indirect_unmarked_callable(void *) { val += 99297; }
}
void (*func)(void *) = indirect_unmarked_callable;
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