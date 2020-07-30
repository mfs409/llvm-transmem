// Test of throw/catch behaviors
//
// Here we start using custom exception types
#include <exception>
struct TestException : public std::exception {
  const char *what() const throw() { return "This is a test exception"; }
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_simple_custom();
int main() {
  report<int>("thrwctch_010", "Catch custom exception with no extra fields",
              {{test_simple_custom(), -99}},
              {{TM_STATS_TRANSLATE_NOTFOUND, 1}, {TM_STATS_UNSAFE, 3}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int catch_simple_custom(int a);
TX_SAFE
int throw_simple_custom(int a) {
  if (a < 0)
    throw TestException();
  else
    return a;
}
int test_simple_custom() {
  int ans = 0;
  TX_BEGIN { ans = catch_simple_custom(-5); }
  TX_END;
  return ans;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int throw_simple_custom(int a);
TX_SAFE
int catch_simple_custom(int a) {
  int val = 0;
  try {
    val = throw_simple_custom(a);
  } catch (TestException te) {
    val = -99;
  }
  return val;
}
#endif
