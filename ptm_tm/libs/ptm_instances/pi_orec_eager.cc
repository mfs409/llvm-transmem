/// Instantiate the P_OrecEager algorithm with the following configuration:
/// - An orec table with 2^20 entries and a coverage of 16 bytes
/// - Coarse-granularity undo logging (orec granularity)
/// - No Quiescence
/// - Hourglass+Backoff contention mgt, since we don't have irrevocability
/// - Hourglass contention management, since we don't have irrevocability
/// - Support for dynamic stack frame optimizations
/// - Allocation with captured memory
/// - ADR persistence domain
///
/// This is the "ideal persistent" implementation: The optimizations are
/// captured memory, no quiescence, and coarse undo logging

// The algorithm we are using:
#include "../ptm_algs/p_orec_eager.h"

// The templates we are using
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/orec_t.h"
#include "../common/p_alloc.h"
#include "../common/p_coarse_undolog.h"
#include "../common/pmem_platform.h"
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
typedef P_OrecEager<
    OrecTable<NUM_STRIPES, 5, CounterTimesource>, P_CoarseUndoLog<32, ADR>,
    BasicEpochManager<MAX_THREADS>,
    HourglassBackoffCM<ABORTS_THRESHOLD, BACKOFF_MIN, BACKOFF_MAX>,
    OptimizedStackFrameManager, EnhancedPersistentAllocationManager<ADR>, ADR>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class O, class U, class E, class C, class S, class A, class P>
typename P_OrecEager<O, U, E, C, S, A, P>::Globals
    P_OrecEager<O, U, E, C, S, A, P>::globals;

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
