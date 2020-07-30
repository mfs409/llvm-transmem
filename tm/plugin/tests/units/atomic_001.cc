// Test of atomic operations.  For TM, atomic operations are UNSAFE.
//
// Atomic exchange() should produce unsafe

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>
std::atomic<int> ato_u4{0};
extern "C" {
int aexch_callable(int);
int tm_aexch_callable(int);
}
int main() {
  report<int>("atomic_001", "atomic exchange should be unsafe",
              {{aexch_callable(0), tm_aexch_callable(0)}},
              {{TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<int> ato_u4;
extern "C" {
TX_SAFE int aexch_callable(int i) { return ato_u4.exchange(i); }
}
#endif

#ifdef TEST_OFILE2
#endif
