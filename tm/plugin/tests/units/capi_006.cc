// Test execution of transactions via the C API
//
// Here we test the use of parameter packets in the C API

struct params_t {
  int a;
  int b;
  int c;
  int res;
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
int tx_call(params_t *);
int main() {
  params_t p = {8, 16, 32};
  report<int>("capi_006", "launch transaction with parameter packet",
              {{tx_call(&p), 56}},
              {{TM_STATS_LOAD_U4, 3}, {TM_STATS_STORE_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
void access_params(void *arg) {
  params_t *p = (params_t *)arg;
  p->res = p->a + p->b + p->c;
}
int tx_call(params_t *p) {
  TX_BEGIN_C(nullptr, access_params, p);
  return p->res;
}
#endif

#ifdef TEST_OFILE2
#endif