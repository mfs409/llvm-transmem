/// Instantiate the P_RingSW algorithm with the following configuration:
/// - 1024-entry ring with 1024-bit SSE filters
/// - Redo log with 64-byte granularity and deferred clwbs
/// - Quiescence
/// - Hourglass+Backoff contention mgt, since we don't have irrevocability
/// - Support for dynamic stack frame optimizations
/// - Allocation with captured memory optimizations
///
/// This is the "general persistent" implementation: the only optimizations we
/// are able to employ are captured memory and deferred clwbs.

// The algorithm we are using:
#include "../ptm_algs/p_ring_sw.h"

// The templates we are using
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/p_alloc.h"
#include "../common/p_redolog.h"
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
typedef P_RingSW<RING_SIZE, SSEBitFilter<RING_FILTER_SIZE, 32>,
                 P_RedoLog<32, false, false>, QuiesceEpochManager<MAX_THREADS>,
                 HourglassBackoffCM<ABORTS_THRESHOLD, BACKOFF_MIN, BACKOFF_MAX>,
                 OptimizedStackFrameManager,
                 EnhancedPersistentAllocationManager>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <int Z, class F, class R, class E, class C, class S, class A>
typename P_RingSW<Z, F, R, E, C, S, A>::Globals
    P_RingSW<Z, F, R, E, C, S, A>::globals;

/// Initialize the API
API_TM_DESCRIPTOR;
API_TM_MALLOC_FREE;
API_TM_MEMFUNCS_GENERIC;
API_TM_LOADFUNCS;
API_TM_STOREFUNCS;
API_TM_STATS_NOP;
API_TM_EXECUTE_NOEXCEPT_PTM;
API_TM_CLONES_THREAD_UNSAFE;
API_TM_STACKFRAME_OPT;
