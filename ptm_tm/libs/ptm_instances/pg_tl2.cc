/// Instantiate the P_TL2 algorithm with the following configuration:
/// - An orec table with 2^20 entries and a coverage of 16 bytes
/// - Redo log with orec granularity and deferred clwbs
/// - Quiescence
/// - Hourglass+Backoff contention mgt, since we don't have irrevocability
/// - Support for dynamic stack frame optimizations
/// - Allocation with captured memory optimizations
/// - Single-checked orec optimization
///
/// This is the "general persistent" implementation: the only optimizations we
/// are able to employ are captured memory and deferred clwbs.

// The algorithm we are using:
#include "../ptm_algs/p_tl2.h"

// The templates we are using
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/orec_t.h"
#include "../common/p_alloc.h"
#include "../common/p_redolog.h"
#include "../common/pmem.h"
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
typedef P_TL2<OrecTable<NUM_STRIPES, 5, CounterTimesource>, P_RedoLog<32, false, false, pmem_adr>,
              QuiesceEpochManager<MAX_THREADS>,
              HourglassBackoffCM<ABORTS_THRESHOLD, BACKOFF_MIN, BACKOFF_MAX>,
              OptimizedStackFrameManager,
              EnhancedPersistentAllocationManager<pmem_adr>, true, pmem_adr>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class O, class R, class E, class C, class S, class A, bool SFO,
          class P>
typename P_TL2<O, R, E, C, S, A, SFO, P>::Globals
    P_TL2<O, R, E, C, S, A, SFO, P>::globals;

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
