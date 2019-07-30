/// Instantiate the P_TLRWEager algorithm with the following configuration:
/// - 2^20 bytelocks, supporting 56 threads max
/// - Coarse-granularity undo logging (bytelock granularity)
/// - No irrevocability, no quiescence
/// - Hourglass+Backoff contention mgt, since we don't have irrevocability
/// - Support for dynamic stack frame optimizations
/// - Allocation with captured memory
/// - Reasonably good deadlock tuning
/// - No store fence pipelining
///
/// This is the "ideal persistent" implementation: The optimizations are
/// captured memory, store pipelining, and coarse undo logging, because we
/// already have no quiescence for TLRW.
///
/// Please see the notes in the corresponding .h file, and in the constants
/// file.  TLRW is extremely sensitive to its tuning parameters, and it is hard
/// to find a configuration that works well all the time.  The numbers we are
/// using, coupled with irrevocability as a fallback contention manager, lead to
/// acceptable performance across all of STAMP.

// The algorithm we are using
#include "../ptm_algs/p_tlrw_eager.h"

// The templates we are using
#include "../common/bytelock_t.h"
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/p_alloc.h"
#include "../common/p_coarse_undolog.h"
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
///
/// NB: P_TLRWEager can livelock with ExpBackoffCM CM on some STAMP benchmarks
typedef P_TLRWEager<
    BytelockTable<NUM_STRIPES, 5, BYTELOCK_MAX_THREADS>, P_CoarseUndoLog<32>,
    BasicEpochManager<MAX_THREADS>,
    HourglassBackoffCM<ABORTS_THRESHOLD, BACKOFF_MIN, BACKOFF_MAX>,
    OptimizedStackFrameManager, EnhancedPersistentAllocationManager,
    TLRW_READ_TRIES, TLRW_READ_SPINS, TLRW_WRITE_TRIES, TLRW_WRITE_SPINS, true>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class O, class U, class E, class C, class S, class A, int RT, int RS,
          int WT, int WS, bool P>
typename P_TLRWEager<O, U, E, C, S, A, RT, RS, WT, WS, P>::Globals
    P_TLRWEager<O, U, E, C, S, A, RT, RS, WT, WS, P>::globals;

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
