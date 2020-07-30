// Test of advanced C++ dispatch facilities
//
// Here we are making sure that a virtual method call to the unmarked child
// class method is unsafe, even if the parent is safe.  This is same-TU.

#ifdef TEST_DRIVER
#include "../include/harness.h"
int test_marked_child_same_tu();
int main() {
  report<int>("dispatch_004",
              "Virtual call to unmarked child in same TU serializes",
              {{test_marked_child_same_tu(), 225}},
              {{TM_STATS_STORE_U4, 0},
               {TM_STATS_UNSAFE, 1},
               {TM_STATS_TRANSLATE_FOUND, 0},
               {TM_STATS_TRANSLATE_NOTFOUND, 1}});
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
  virtual void set(int i);
  virtual ~child() {}
};
TX_SAFE void parent::set(int i) { x = i; }
void child::set(int i) { x = i * i; }

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