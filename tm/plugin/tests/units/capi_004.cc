// Test execution of transactions via the C API
//
// Here we ensure that passing an unmarked C function in the same TU to the
// BEGIN_C function does not cause serialization

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int tx_call_it();
}

int main() {
  report<int>("capi_004",
              "Start transaction from unmarked C function in same TU",
              {{tx_call_it(), 1}},
              {{TM_STATS_UNSAFE, 0},
               {TM_STATS_BEGIN_OUTER, 1},
               {TM_STATS_LOAD_U4, 1},
               {TM_STATS_STORE_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"

int a = 0;
extern "C" {
void inc(void *) { a++; }
int tx_call_it() {
  TX_BEGIN_C(nullptr, inc, nullptr);
  return a;
}
}
#endif

#ifdef TEST_OFILE2
#endif