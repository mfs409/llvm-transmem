// Test of C++ intrinsics
//
// here we are making sure that memset gets instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
int test_memset();
int tm_test_memset();
}

int main() {
  // NB: As in clone_001, the real test is "did the code link" :)
  report<int>("intrin_002", "llvm memset intrinsic gets instrumented",
              {{test_memset(), tm_test_memset()}}, {{TM_STATS_MEMSET, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cstring>
extern "C" {
TX_SAFE int test_memset() {
  char str[] = "Transactional Memory Interface.";
  memset(str, '-', 6);
  int ans = 0;
  for (int i = 0; i < 6; ++i)
    ans += str[i];
  return ans;
}
}
#endif

#ifdef TEST_OFILE2
#endif