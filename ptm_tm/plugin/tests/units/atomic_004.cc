// Test of atomic operations.  For TM, atomic operations are UNSAFE.
//
// Memory fences should be safe, since they don't have a memory effect

#ifdef TEST_DRIVER
#include "../include/harness.h"
extern "C" {
int fence_callable();
int tm_fence_callable();
}
int main() {
  report<int>("atomic_004", "Memory fences should not produce unsafe",
              {{fence_callable(), tm_fence_callable()}},
              {{TM_STATS_UNSAFE, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern "C" {
TX_SAFE int fence_callable() {
  std::atomic_thread_fence(std::memory_order_seq_cst);
  return 0;
}
}
#endif

#ifdef TEST_OFILE2
#endif