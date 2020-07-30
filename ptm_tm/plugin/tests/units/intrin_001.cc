// Test of C++ intrinsics
//
// here we are making sure that memcpy gets instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
int test_memcpy();
int tm_test_memcpy();
}

int main() {
  report<int>("intrin_001", "llvm memcpy intrinsic gets instrumented",
              {{test_memcpy(), tm_test_memcpy()}}, {{TM_STATS_MEMCPY, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cstring>
extern "C" {
int a[3] = {1, 2, 3};
TX_SAFE int test_memcpy() {
  int *p = (int *)malloc(3 * sizeof(int));
  memcpy(p, a, 3 * sizeof(int));
  int ans;
  for (int i = 0; i < 3; ++i)
    ans = p[i];
  free(p);
  return ans;
}
}
#endif

#ifdef TEST_OFILE2
#endif