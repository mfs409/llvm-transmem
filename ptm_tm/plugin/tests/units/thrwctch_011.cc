// Test of throw/catch behaviors
//
// Make sure that we can deal with custom exception types on the heap

#include <exception>
struct TestException : public std::exception {
  const char *what() const throw() { return "This is a test exception"; }
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_simple_custom_heap();
int main() {
  report<int>("thrwctch_011", "Custom exception on heap",
              {{test_simple_custom_heap(), -99}},
              {{TM_STATS_TRANSLATE_NOTFOUND, 1}, {TM_STATS_UNSAFE, 4}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int catch_simple_custom_heap(int a);
TX_SAFE
int throw_simple_custom_heap(int a) {
  if (a < 0)
    throw new TestException();
  else
    return a;
}
int test_simple_custom_heap() {
  int ans = 0;
  TX_BEGIN { ans = catch_simple_custom_heap(-5); }
  TX_END;
  return ans;
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
int throw_simple_custom_heap(int a);
TX_SAFE
int catch_simple_custom_heap(int a) {
  int val = 0;
  try {
    val = throw_simple_custom_heap(a);
  } catch (TestException *te) {
    val = -99;
  }
  return val;
}
#endif
