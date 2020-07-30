// Test of throw/catch behaviors
//
// Here we look at what happens when the throw and catch are in the same
// function.  Since clang does not put LLVM exception IR into the bitcode, this
// is really a test of unsafeness, so we can't really look at much.

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_throw_catch_same_func();
int main() {
  report<int>("thrwctch_001",
              "throw and catch inside of same function, integer exception",
              {{test_throw_catch_same_func(), -1}}, {{TM_STATS_UNSAFE, 2}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
TX_SAFE
int throw_catch_same_func(int a) {
  int res = a * 2;
  try {
    if (a < 0)
      throw - 1;
    if (a < 50)
      throw 77;
    res *= 50 * 77;
  } catch (int i) {
    res = i;
  }
  return res;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int throw_catch_same_func(int a);
int test_throw_catch_same_func() {
  int ans = 0;
  TX_BEGIN { ans = throw_catch_same_func(-5); }
  TX_END;
  return ans;
}
#endif