// Test of per-BB TM-specific optimizations
//
// Unsafe across BBs won't get optimized yet

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
int test_unsafe(int);
int tm_test_unsafe(int);
}

int main() {
  report<int>("unsfopt_002",
              "Dominance of UNSAFE calls doesn't get optimized away",
              {{test_unsafe(5), tm_test_unsafe(5)}}, {{TM_STATS_UNSAFE, 3}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
void empty(int);
std::atomic<int> ai = {0};
extern "C" {
TX_SAFE int test_unsafe(int v) {
  ai = v; // unsafe, dominates, kept
  if (v < 0) {
    empty(17);
    ai = 88; // unsafe, dominated, kept
  } else {
    empty(16);
    ai = 16; // unsafe, dominated, kept
  }
  return ai; // unsafe, dominated, kept
}
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
TX_SAFE void empty(int) {}
#endif