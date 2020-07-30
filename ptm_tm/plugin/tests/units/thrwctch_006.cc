// Test of throw/catch behaviors
//
// Test use of "..." in catch statements

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_catch_ellipses();
int main() {
  report<int>("thrwctch_006", "Catch with ellipses",
              {{test_catch_ellipses(), -99}},
              {{TM_STATS_UNSAFE, 4}, {TM_STATS_TRANSLATE_NOTFOUND, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int throw_std_exception(int a);
TX_SAFE
int catch_ellipses(int a) {
  int val = 0;
  try {
    val = throw_std_exception(a);
  } catch (...) {
    val = -99;
  }
  return val;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int catch_ellipses(int a);
int test_catch_ellipses() {
  int ans = 0;
  TX_BEGIN { ans = catch_ellipses(-5); }
  TX_END;
  return ans;
}
TX_SAFE
int throw_std_exception(int a) {
  if (a < 0)
    throw std::runtime_error("unknown error");
  else
    return a;
}
#endif