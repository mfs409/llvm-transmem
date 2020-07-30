// Test of per-BB TM-specific optimizations
//
// Unsafe from translate_notfound won't get optimized away

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
int test_unsafe(int);
int tm_test_unsafe(int);
}

int main() {
  report<int>("unsfopt_003",
              "Calls to unsafe from TRANSLATE don't get optimized away",
              {{test_unsafe(5), tm_test_unsafe(5)}}, {{TM_STATS_UNSAFE, 6}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
void empty(int);
std::atomic<int> ai = {0};
extern "C" {
TX_SAFE int test_unsafe(int v) {
  empty(1);  // unsafe but compiler doesn't know.
  empty(2);  // unsafe but compiler doesn't know.
  empty(3);  // unsafe but compiler doesn't know.
  empty(4);  // unsafe but compiler doesn't know.
  empty(5);  // unsafe but compiler doesn't know.
  return ai; // unsafe, kept
}
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
void empty(int) {}
#endif