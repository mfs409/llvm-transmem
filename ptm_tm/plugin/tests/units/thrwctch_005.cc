// Test of throw/catch behaviors
//
// Use a std::exception to do throw and catch

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_catch_std_exception();
int main() {
  report<int>("thrwctch_005", "Throw and catch a std::exception",
              {{test_catch_std_exception(), -99}},
              {{TM_STATS_UNSAFE, 7}, {TM_STATS_TRANSLATE_NOTFOUND, 4}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int catch_std_exception(int a);
TX_SAFE
int throw_std_exception(int a) {
  if (a < 0)
    throw std::runtime_error("unknown error");
  return a;
}
int test_catch_std_exception() {
  int ans = 0;
  TX_BEGIN { ans = catch_std_exception(-5); }
  TX_END;
  return ans;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int throw_std_exception(int a);
TX_SAFE
int catch_std_exception(int a) {
  int val = 0;
  try {
    val = throw_std_exception(a);
  } catch (std::runtime_error) {
    val = -99;
  }
  return val;
}
#endif