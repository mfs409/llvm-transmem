// Test of advanced C++ dispatch facilities
//
// Here we are making sure that a virtual method call to the marked child class
// method is safe, even if the parent is not.  This is same-TU.

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_marked_child_same_tu();
int main() {
  report<int>("dispatch_002", "Virtual call to marked child in same TU works",
              {{test_marked_child_same_tu(), 225}},
              {{TM_STATS_STORE_U4, 1},
               {TM_STATS_UNSAFE, 0},
               {TM_STATS_TRANSLATE_FOUND, 1},
               {TM_STATS_TRANSLATE_NOTFOUND, 0}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"

// parent method unsafe, child method safe
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
void parent::set(int i) { x = i; }
TX_SAFE void child::set(int i) { x = i * i; }

parent *p = new parent();
child *c = new child();

int test_marked_child_same_tu() {
  c->x = 6;
  TX_BEGIN { c->set(15); }
  TX_END;
  return c->x;
}
#endif

#ifdef TEST_OFILE2
#endif