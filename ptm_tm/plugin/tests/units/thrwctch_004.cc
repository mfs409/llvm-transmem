// Test of throw/catch behaviors
//
// When a transaction throws, and the exception is supposed to escape the
// transaction, we should have one unsafe for the throw, and the transaction
// should still commit.
//
// NB: "the transaction should still commit" is something that our actual TM
//     libraries may fail to do, e.g., if they are instantiated with
//     "NOEXCEPT" macros.  This test uses CountingTM, which is
//     exception-safe, so this test should pass.

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_catch_outside_tx();
int main() {
  report<int>("thrwctch_004", "Catch outside of transaction",
              {{test_catch_outside_tx(), 0}},
              {{TM_STATS_UNSAFE, 1},
               {TM_STATS_END_OUTER, 1},
               {TM_STATS_TRANSLATE_NOTFOUND, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
TX_SAFE
int throw_dont_catch(int a) {
  int res = a * 2;
  if (a < 0)
    throw 1;
  return res;
}
#endif

#ifdef TEST_OFILE2
#include <iostream>

#include "../../../common/tm_api.h"
int throw_dont_catch(int a);
int test_catch_outside_tx() {
  try {
    TX_BEGIN { throw_dont_catch(-7); }
    TX_END; // should still commit
  } catch (int &i) {
    if (i != 1) {
      std::cout << "Error: {1} != {" << i << "}\n";
    }
  }
  return 0;
}
#endif
