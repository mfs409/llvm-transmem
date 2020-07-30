// Test discovery of functions invoked from an instrumented region.
//
// Ensure that invoking a marked function in another TU works

struct exception {
  char tmp;
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
int something = 0;
extern "C" {
int marked_invoke_marked_outside_tu(int);
int tm_marked_invoke_marked_outside_tu(int);
}

int main() {
  report<int>("invoke_005", "invoke to marked function in another TU",
              {{marked_invoke_marked_outside_tu(54),
                tm_marked_invoke_marked_outside_tu(54)}},
              {{TM_STATS_TRANSLATE_FOUND, 1}, {TM_STATS_UNSAFE, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"

extern "C" {
int marked_in_other_tu_invokable(int);
TX_SAFE int marked_invoke_marked_outside_tu(int i) {
  try {
    return marked_in_other_tu_invokable(i);
  } catch (exception any) {
    return -1;
  }
}
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
extern int something;
extern "C" {
TX_SAFE int marked_in_other_tu_invokable(int i) {
  if (i == -1024) {
    throw exception();
  }
  return i * something + something;
}
}
#endif