// Test of throw/catch behaviors
//
// Make sure that we can return from a catch statement

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_return_from_catch();
int main() {
  report<int>("thrwctch_003", "Return from a catch statement",
              {{test_return_from_catch(), 81}}, {{TM_STATS_UNSAFE, 2}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int return_from_catch(int a);
TX_SAFE
int throw_dont_catch(int a) {
  int res = a * 2;
  if (a < 0)
    throw - 9;
  return res;
}
int test_return_from_catch() {
  int ans = 0;
  TX_BEGIN { ans = return_from_catch(-5); }
  TX_END;
  return ans;
}

#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int throw_dont_catch(int a);
TX_SAFE
int return_from_catch(int a) {
  int val = 0;
  try {
    val = throw_dont_catch(a);
  } catch (int i) {
    return i * i;
  }
  val *= a;
  return val;
}
#endif
