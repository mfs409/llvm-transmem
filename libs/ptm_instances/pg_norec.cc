/// Instantiate the P_NOrec algorithm with the following configuration:
/// - Redo log with 32-byte granularity and deferred clwb
/// - Quiescence
/// - Hourglass+Backoff contention mgt, since we don't have irrevocability
/// - Support for dynamic stack frame optimizations
/// - Allocation with captured memory optimization
///
/// This is the "general persistent" implementation: the only optimizations we
/// are able to employ are captured memory and deferred clwbs.

// The algorithm we are using:
#include "../ptm_algs/p_norec.h"

// The templates we are using
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/p_alloc.h"
#include "../common/p_redolog.h"
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
typedef P_NOrec<P_RedoLog<32, false, false>, QuiesceEpochManager<MAX_THREADS>,
		HourglassBackoffCM<ABORTS_THRESHOLD, BACKOFF_MIN, BACKOFF_MAX>,
		OptimizedStackFrameManager, EnhancedPersistentAllocationManager>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class R, class E, class C, class S, class A>
typename P_NOrec<R, E, C, S, A>::Globals P_NOrec<R, E, C, S, A>::globals;

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
