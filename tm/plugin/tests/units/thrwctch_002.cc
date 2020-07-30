// Test of throw/catch behaviors
//
// Here we look at throwing in one function, catching in another, across TUs

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_throw_dont_catch();
int main() {
  report<int>("thrwctch_002", "Throwing in one TU, catching in another",
              {{test_throw_dont_catch(), -9}}, {{TM_STATS_UNSAFE, 2}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
int catch_from_other(int a);
int tm_catch_from_other(int a);
TX_SAFE
int throw_dont_catch(int a) {
  int res = a * 2;
  if (a < 0)
    throw - 9;
  return res;
}
}
int test_throw_dont_catch() {
  int ans = 0;
  TX_BEGIN { ans = catch_from_other(-5); }
  TX_END;
  return ans;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
extern "C" {
int throw_dont_catch(int a);
TX_SAFE int catch_from_other(int a) {
  int val = 0;
  try {
    val = throw_dont_catch(a);
  } catch (int i) {
    val = i;
  }
  return val;
}
}
#endif