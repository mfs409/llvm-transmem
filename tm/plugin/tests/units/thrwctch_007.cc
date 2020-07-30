// Test of throw/catch behaviors
//
// Make sure that we can return from a catch statement

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_rethrow();
int main() {
  report<int>("thrwctch_007", "Return from a catch statement",
              {{test_rethrow(), -7}},
              {{TM_STATS_UNSAFE, 9}, {TM_STATS_TRANSLATE_NOTFOUND, 4}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int throw_std_exception(int a);
TX_SAFE
int rethrow_it(int a) {
  int val = 0;
  try {
    val = throw_std_exception(a);
  } catch (std::runtime_error r) {
    if (a < 0)
      throw;
  }
  return val;
}

#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int rethrow_it(int a);
TX_SAFE
int throw_std_exception(int a) {
  if (a < 0)
    throw std::runtime_error("unknown error");
  return a;
}
int test_rethrow() {
  int val = 0;
  try {
    TX_BEGIN { rethrow_it(-7); }
    TX_END;
  } catch (...) {
    val = -7;
  }
  return val;
}

#endif
