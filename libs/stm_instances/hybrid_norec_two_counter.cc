/// Instantiate HybridNOrecTwoCounter with the following configuration:
/// - 8 Attempts before STM fallback
/// - 32-byte redo log granularity
/// - Irrevocability and Quiescence
/// - No contention management
/// - Support for dynamic stack frame optimizations
/// - Irrevocability after 128 mallocs, capture optimizations

// The algorithm we are using:
#include "../stm_algs/hybrid_norec_two_counter.h"

// The templates we are using
#include "../common/alloc.h"
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/redolog.h"
#include "../common/stackframe.h"

// The API generators
#include "../api/clone.h"
#include "../api/execute.h"
#include "../api/frame.h"
#include "../api/loadstore.h"
#include "../api/mem.h"
#include "../api/stats.h"

// Common constants when instantiating
#include "../api/constants.h"

/// TxThread is a shorthand for the instantiated version of the TM algorithm, so
/// that we can use common macros to define the API:
typedef HybridNOrecTwoCounter<NUM_HTM_RETRIES, RedoLog<32>,
                              IrrevocQuiesceEpochManager<MAX_THREADS>, NoopCM,
                              OptimizedStackFrameManager,
                              BoundedAllocationManager<MALLOC_THRESHOLD, true>>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <int RT, class R, class E, class C, class S, class A>
typename HybridNOrecTwoCounter<RT, R, E, C, S, A>::Globals
    HybridNOrecTwoCounter<RT, R, E, C, S, A>::globals;

/// Initialize the API.  Be sure to use Hybrid EXECUTE path
API_TM_DESCRIPTOR;
API_TM_MALLOC_FREE;
API_TM_MEMFUNCS_GENERIC;
API_TM_LOADFUNCS;
API_TM_STOREFUNCS;
API_TM_STATS_NOP;
API_TM_EXECUTE_HYBRID;
API_TM_CLONES_THREAD_UNSAFE;
API_TM_STACKFRAME_OPT;