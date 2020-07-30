// Test of malloc/free
//
// Here we are making sure that aligned_alloc gets instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
int test_aligned_alloc(int);
int tm_test_aligned_alloc(int);
}

int main() {
  report<bool>("align_001", "calls to aligned_alloc get instrumented",
               {{test_aligned_alloc(0), tm_test_aligned_alloc(1)}},
               {{TM_STATS_UNSAFE, 0}, {TM_STATS_ALIGNED_ALLOC, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cstdlib>

void *my_pointer[2];
extern "C" {
TX_SAFE bool test_aligned_alloc(int idx) {
  void *tmp = aligned_alloc(64, 1024);
  my_pointer[idx] = tmp;
  return my_pointer[idx] != nullptr;
}
}
#endif

#ifdef TEST_OFILE2
#endif
