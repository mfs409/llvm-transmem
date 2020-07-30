// Test of malloc/free
//
// Here we are making sure that free() gets instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
bool test_free(int);
bool tm_test_free(int);
}

int main() {
  report<bool>("alloc_002", "calls to free get instrumented",
               {{test_free(0), tm_test_free(1)}},
               {{TM_STATS_UNSAFE, 0}, {TM_STATS_FREES, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cstdlib>

void *my_pointer[2] = {malloc(7), malloc(9)};
extern "C" {
TX_SAFE bool test_free(int idx) {
  free(my_pointer[idx]);
  return true;
}
}
#endif

#ifdef TEST_OFILE2
#endif
