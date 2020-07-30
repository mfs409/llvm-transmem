// Test discovery of functions invoked from an instrumented region.
//
// Ensure that invoking a marked function in the same TU works

struct exception {
  char tmp;
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
int something = 0;
extern "C" {
int marked_invoke_marked(int i);
int tm_marked_invoke_marked(int i);
}

int main() {
  report<int>("invoke_001", "invoke to marked function in same TU",
              {{marked_invoke_marked(99), tm_marked_invoke_marked(99)}},
              {{TM_STATS_LOAD_U4, 1}, {TM_STATS_UNSAFE, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern int something;
extern "C" {
TX_SAFE int marked_func_invokable(int i) { return i + something; }
TX_SAFE int marked_invoke_marked(int i) {
  int res = 0;
  try {
    res = marked_func_invokable(i);
  } catch (exception any) {
    res = -1;
  }
  return res;
}
}
#endif

#ifdef TEST_OFILE2
#endif