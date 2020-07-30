// Test of atomic operations.  For TM, atomic operations are UNSAFE.
//
// Atomic CAS should produce unsafe

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>
std::atomic<int> ato_u4{0};
extern "C" {
int acas_callable(int, int);
int tm_acas_callable(int, int);
}
int main() {
  report<int>("atomic_002", "atomic cas should be unsafe",
              {{acas_callable(0, 0), tm_acas_callable(0, 0)}},
              {{TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<int> ato_u4;
extern "C" {
TX_SAFE int acas_callable(int expect, int i) {
  return ato_u4.compare_exchange_strong(expect, i);
}
}
#endif

#ifdef TEST_OFILE2
#endif