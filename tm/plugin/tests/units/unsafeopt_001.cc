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
  report<int>("unsfopt_001",
              "Atomic ops create extra BBs, so we can't coalesce unsafe calls",
              {{test_unsafe(5), tm_test_unsafe(5)}}, {{TM_STATS_UNSAFE, 6}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
std::atomic<int> ai = {0};
extern "C" {
TX_SAFE int test_unsafe(int v) {
  ai = v;
  ai++;
  ai++;
  ai++;
  ai++;
  return ai;
}
}
#endif

#ifdef TEST_OFILE2
#endif
