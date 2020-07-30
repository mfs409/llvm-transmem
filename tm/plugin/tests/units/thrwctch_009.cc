// Test of throw/catch behaviors
//
// Check for widening the type that is caught

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_wide_catch();
int main() {
  report<int>("thrwctch_009", "Catch parent class", {{test_wide_catch(), 10}},
              {{TM_STATS_TRANSLATE_NOTFOUND, 3}, {TM_STATS_UNSAFE, 6}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
TX_SAFE
int throw_std_exception(int a) {
  if (a < 0)
    throw std::runtime_error("unknown error");
  return a;
}
int wide_catch(int a);
int test_wide_catch() {
  int val = 0;
  try {
    TX_BEGIN { val = wide_catch(-7); }
    TX_END;
  } catch (...) {
    val = -7;
  }
  return val;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int throw_std_exception(int a);
TX_SAFE
int wide_catch(int a) {
  int val = 0;
  try {
    val = throw_std_exception(a);
  } catch (std::exception) {
    return 10;
  }
  return val;
}
#endif
