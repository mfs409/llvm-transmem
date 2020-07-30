// Test of advanced C++ dispatch facilities.
//
// Here we are making sure that a virtual method call to the marked parent
// class method is safe.  Parent is defined in the same TU.

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_marked_parent_same_tu();
int main() {
  report<int>("dispatch_003", "Virtual call to marked parent in same TU works",
              {{test_marked_parent_same_tu(), 15}},
              {{TM_STATS_STORE_U4, 1},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_TRANSLATE_FOUND, 1},
               {TM_STATS_TRANSLATE_NOTFOUND, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"

struct parent {
  int x;
  virtual void set(int i);
  virtual ~parent() {}
};
struct child : virtual parent {
  TX_SAFE
  virtual void set(int i);
  virtual ~child() {}
};
TX_SAFE void parent::set(int i) { x = i; }
void child::set(int i) { x = i * i; }

parent *p = new parent();
child *c = new child();

int test_marked_parent_same_tu() {
  p->x = 6;
  TX_BEGIN { p->set(15); }
  TX_END;
  return p->x;
}
#endif

#ifdef TEST_OFILE2
#endif