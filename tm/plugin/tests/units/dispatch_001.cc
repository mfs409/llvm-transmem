// Test of advanced C++ dispatch facilities.
//
// Here we are making sure that a virtual method call to the unmarked parent
// class method is unsafe, even when parent is defined in the same TU.

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_unmarked_parent_same_tu();
int main() {
  report<int>("dispatch_001",
              "Virtual call to unmarked parent in same TU serializes",
              {{test_unmarked_parent_same_tu(), 15}},
              {{TM_STATS_STORE_U4, 0},
               {TM_STATS_UNSAFE, 1},
               {TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 1}});
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

int test_unmarked_parent_same_tu() {
  p->x = 6;
  TX_BEGIN { p->set(15); }
  TX_END;
  return p->x;
}
#endif

#ifdef TEST_OFILE2
#endif