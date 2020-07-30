// Test of malloc/free
//
// Here we are making sure that malloc() gets instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
int test_malloc(int);
int tm_test_malloc(int);
}

int main() {
  report<bool>("alloc_001", "calls to malloc get instrumented",
               {{test_malloc(0), tm_test_malloc(1)}},
               {{TM_STATS_UNSAFE, 0}, {TM_STATS_MALLOCS, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cstdlib>

void *my_pointer[2];
extern "C" {
TX_SAFE bool test_malloc(int idx) {
  void *tmp = malloc(1024);
  my_pointer[idx] = tmp;
  return my_pointer[idx] != nullptr;
}
}
#endif

#ifdef TEST_OFILE2
#endif
