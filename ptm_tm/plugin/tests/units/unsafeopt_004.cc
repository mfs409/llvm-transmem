// Test of per-BB TM-specific optimizations
//
// Our test has one BB, which has several unsafe operations.  We should only
// have one UNSAFE, though.

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
int test_unsafe(int);
int tm_test_unsafe(int);
}

int main() {
  report<int>("unsfopt_004",
              "Multiple volatile ops in one BB should cause one unsafe call",
              {{test_unsafe(5), tm_test_unsafe(5)}}, {{TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
volatile int vi = {0};
extern "C" {
TX_SAFE int test_unsafe(int v) {
  vi = v;
  vi++;
  vi++;
  vi++;
  vi++;
  return vi;
}
}
#endif

#ifdef TEST_OFILE2
#endif
