// Test of throw/catch behaviors
//
// Ensure that we bypass catch blocks that aren't appropriate for the exception

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_wrong_catch();
int main() {
  report<int>("thrwctch_008", "Skip inappropriate catch blocks",
              {{test_wrong_catch(), -7}},
              {{TM_STATS_UNSAFE, 3}, {TM_STATS_TRANSLATE_NOTFOUND, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int throw_std_exception(int a);
TX_SAFE
int wrong_catch(int a) {
  int val = 0;
  try {
    val = throw_std_exception(a);
  } catch (std::bad_cast) {
    return 10;
  }
  return val;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int wrong_catch(int a);
TX_SAFE
int throw_std_exception(int a) {
  if (a < 0)
    throw std::runtime_error("unknown error");
  return a;
}
int test_wrong_catch() {
  int val = 0;
  try {
    TX_BEGIN { wrong_catch(-7); }
    TX_END;
  } catch (...) {
    val = -7;
  }
  return val;
}
#endif
