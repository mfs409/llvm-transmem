/// Instantiate the HybridRingSW algorithm with the following configuration:
/// - 8 Attempts before STM fallback
/// - 1024-entry ring with 1024-bit SSE filters
/// - Redo log with 64-byte chunks
/// - Irrevocability and Quiescence
/// - No contention management
/// - Support for dynamic stack frame optimizations
/// - Irrevocability after 128 mallocs, captured memory

// The algorithm we are using:
#include "../stm_algs/hybrid_ring_sw.h"

// The templates we are using
#include "../common/alloc.h"
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/redolog_nonatomic.h"
#include "../common/sse_bitfilter.h"
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
typedef HybridRingSW<NUM_HTM_RETRIES, RING_SIZE,
                SSEBitFilter<RING_FILTER_SIZE, RING_COVERAGE>,
                RedoLog_Nonatomic<2 << RING_COVERAGE>,
                IrrevocQuiesceEpochManager<MAX_THREADS>, NoopCM,
                OptimizedStackFrameManager,
                BoundedAllocationManager<MALLOC_THRESHOLD, true>>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <int RT,int Z, class F, class R, class E, class C, class S, class A>
typename HybridRingSW<RT, Z, F, R, E, C, S, A>::Globals
    HybridRingSW<RT, Z, F, R, E, C, S, A>::globals;

/// Initialize the API
API_TM_DESCRIPTOR;
API_TM_MALLOC_FREE;
API_TM_MEMFUNCS_GENERIC;
API_TM_LOADFUNCS;
API_TM_STOREFUNCS;
API_TM_STATS_NOP;
API_TM_EXECUTE_HYBRID;
API_TM_CLONES_THREAD_UNSAFE;
API_TM_STACKFRAME_OPT;