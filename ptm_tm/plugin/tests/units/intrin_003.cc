// Test of C++ intrinsics
//
// Here we are making sure that memmove gets instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
int test_memmove();
int tm_test_memmove();
}

int main() {
  report<int>("intrin_003", "llvm memmove intrinsic gets instrumented",
              {{test_memmove(), tm_test_memmove()}}, {{TM_STATS_MEMMOVE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cstring>
extern "C" {
char str[] = "1234";
TX_SAFE int test_memmove() {
  str[3] = '4';
  str[2] = '3';
  str[1] = '2';
  str[0] = '1';
  memmove(str + 2, str + 1, 2);
  int ans = 0;
  for (int i = 0; i < 4; ++i)
    ans += str[i];
  return ans;
}
}
#endif

#ifdef TEST_OFILE2
#endif