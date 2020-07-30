// Test of LLVM IR for vector loads and stores
//
// When we explicitly use sse data types, we should end up with the sort of
// vector instructions that lead to not-yet-supported 128-bit+ loads and stores

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
float do_sqrt_sse(float *, int);
float tm_do_sqrt_sse(float *, int);
}

int main() {
  int N = 4;
  float *a;
  posix_memalign((void **)&a, 16, N * sizeof(float));
  for (int i = 0; i < N; ++i)
    a[i] = i;
  float r1 = do_sqrt_sse(a, N);
  for (int i = 0; i < N; ++i)
    a[i] = i;
  float r2 = tm_do_sqrt_sse(a, N);
  report<int>(
      "ssevec_002", "SSE vectors produce unsafe loads and stores", {{r1, r2}},
      {{TM_STATS_LOAD_F, 4}, {TM_STATS_STORE_F, 0}, {TM_STATS_UNSAFE, 2}});
}

#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cmath>
#include <emmintrin.h>
extern "C" {
TX_SAFE float do_sqrt_sse(float *a, int N) {
  int nb_iters = N / 4; // We assume N % 4 == 0.
  __m128 *ptr = (__m128 *)a;
  for (int i = 0; i < nb_iters; ++i, ++ptr, a += 4)
    _mm_store_ps(a, _mm_sqrt_ps(*ptr));
  float sum = 0;
  for (int i = 0; i < N; ++i)
    sum = sum + a[i];
  return sum;
}
}
#endif

#ifdef TEST_OFILE2
#endif