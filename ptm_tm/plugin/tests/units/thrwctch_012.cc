// Test of throw/catch behaviors
//
// Here we look at what happens when the exception has custom fields

#include <exception>
struct TestExceptionWithExtra : public std::exception {
  int extra;
  const char *what() const throw() {
    return "Be sure to check the 'extra' field";
  }
  TestExceptionWithExtra(int i) : extra(i) {}
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_custom();
int main() {
  report<int>("thrwctch_012", "custom exception with extra fields",
              {{test_custom(), -99}},
              {{TM_STATS_TRANSLATE_NOTFOUND, 1}, {TM_STATS_UNSAFE, 4}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern int data;
int catch_custom(int a);
TX_SAFE
int throw_custom(int a) {
  if (a < 0)
    throw new TestExceptionWithExtra(data);
  else
    return a;
}
int test_custom() {
  int ans = 0;
  TX_BEGIN { ans = catch_custom(-5); }
  TX_END;
  return ans;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int data = 99;
int throw_custom(int a);
TX_SAFE
int catch_custom(int a) {
  int val = 0;
  try {
    val = throw_custom(a);
  } catch (TestExceptionWithExtra *te) {
    val = -99;
  }
  return val;
}
#endif
