// Test execution of transactions via the C API
//
// Here we ensure that nested C API transactions behave correctly

#ifdef TEST_DRIVER
#include "../include/harness.h"

int call_outer();

int main() {
  report<int>("capi_003", "Nested C API regions in same TU",
              {{call_outer(), 3}},
              {{TM_STATS_UNSAFE, 0},
               {TM_STATS_BEGIN_OUTER, 1},
               {TM_STATS_END_OUTER, 1},
               {TM_STATS_BEGIN_INNER, 1},
               {TM_STATS_END_INNER, 1},
               {TM_STATS_STORE_U4, 3},
               {TM_STATS_LOAD_U4, 3}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int val = 0;
TX_SAFE
void inner(void *) { val++; }
TX_SAFE
void outer(void *) {
  val++;
  TX_BEGIN_C(nullptr, inner, nullptr);
  val++;
}
int call_outer() {
  TX_BEGIN_C(nullptr, outer, nullptr);
  return val;
}
#endif

#ifdef TEST_OFILE2
#endif