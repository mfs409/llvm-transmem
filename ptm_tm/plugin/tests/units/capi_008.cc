// Test execution of transactions via the C API
//
// Here we ensure that using an un-annotated function as the root of a
// transaction leads to serializing if the annotated function is in another TU

#ifdef TEST_DRIVER
#include "../include/harness.h"
int call_it(void *);
int tx_call_it(void *);
int main() {
  report<int>("capi_008",
              "Start from unmarked C function in other TU serializes",
              {{call_it(nullptr), tx_call_it(nullptr)}},
              {{TM_STATS_BEGIN_OUTER, 1},
               {TM_STATS_END_OUTER, 1},
               {TM_STATS_LOAD_U4, 0},
               {TM_STATS_STORE_U4, 0},
               {TM_STATS_UNSAFE, 1},
               {TM_STATS_TRANSLATE_NOTFOUND, 1},
               {TM_STATS_TRANSLATE_FOUND, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int val = 0;
extern "C" {
void incval(void *);
}
int tx_call_it(void *param) {
  val = 73;
  TX_BEGIN_C(nullptr, incval, param);
  return val;
}
int call_it(void *param) {
  val = 73;
  incval(param);
  return val;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
extern int val;
extern "C" {
void incval(void *) { val++; }
}
#endif