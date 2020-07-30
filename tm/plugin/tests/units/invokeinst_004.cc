// Test discovery of functions invoked from an instrumented region.
//
// Confirm that discovery via invokeinst doesn't work across TU boundaries

struct exception {
  char tmp;
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
int something = 0;
extern "C" {
int marked_invoke_unmarked_outside_tu(int);
int tm_marked_invoke_unmarked_outside_tu(int);
}

int main() {
  report<int>("invoke_004", "unsafe invoke to unmarked function in another TU",
              {{marked_invoke_unmarked_outside_tu(74),
                tm_marked_invoke_unmarked_outside_tu(74)}},
              {{TM_STATS_TRANSLATE_NOTFOUND, 1}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"

int unmarked_in_other_tu_invokable(int i);
extern "C" {
TX_SAFE int marked_invoke_unmarked_outside_tu(int i) {
  try {
    return unmarked_in_other_tu_invokable(i);
  } catch (exception any) {
    return -1;
  }
}
}
#endif

#ifdef TEST_OFILE2
extern int something;
int unmarked_in_other_tu_invokable(int i) { return i * something * something; }
#endif