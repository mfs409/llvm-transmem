// Test discovery of functions invoked from an instrumented region.
//
// Ensure that invoking a function that calls an unmarked in the same TU leads
// to the unmarked being cloned

struct exception {
  char tmp;
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
int something = 0;
extern "C" {
int tm_unmarked_in_tu_invokable(int);
int unmarked_in_tu_invokable(int);
}
int main() {
  report<int>("invoke_002", "marked function invokes unmarked function",
              {{unmarked_in_tu_invokable(88), tm_unmarked_in_tu_invokable(88)}},
              {{TM_STATS_LOAD_U4, 1}, {TM_STATS_UNSAFE, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern int something;
extern "C" {
int unmarked_in_tu_invokable(int i) { return i * something; }
TX_SAFE int marked_invoke_unmarked_in_tu(int i) {
  try {
    return unmarked_in_tu_invokable(i);
  } catch (exception any) {
    return -1;
  }
}
}
#endif

#ifdef TEST_OFILE2
#endif