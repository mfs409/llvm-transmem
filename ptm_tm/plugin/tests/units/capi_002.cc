// Test execution of transactions via the C API
//
// Here we make sure that a transaction's root can be a function in the same TU
// that isn't extern C

#ifdef TEST_DRIVER
#include "../include/harness.h"

int val;

extern "C" {
int call_it(void *);
int tx_call_it(void *);
}

int main() {
  report<int>("capi_002", "Start from marked C++ function in same TU",
              {{call_it(nullptr), tx_call_it(nullptr)}},
              {{TM_STATS_BEGIN_OUTER, 1},
               {TM_STATS_LOAD_U4, 1},
               {TM_STATS_END_OUTER, 1},
               {TM_STATS_UNSAFE, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern int val;
TX_SAFE void incval(void *) { val++; }
extern "C" {
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
}
#endif

#ifdef TEST_OFILE2
#endif