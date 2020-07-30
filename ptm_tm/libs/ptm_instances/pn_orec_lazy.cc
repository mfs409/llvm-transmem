/// Instantiate the P_OrecLazy algorithm with the following configuration:
/// - An orec table with 2^20 entries and a coverage of 16 bytes
/// - Redo log with orec granularity and naive (early) clwb
/// - Quiescence
/// - Exponential Backoff for contention management
/// - Support for dynamic stack frame optimizations
/// - Standard allocation
/// - ADR persistence domain
///
/// This is the "naive persistent" implementation: it doesn't have any
/// optimizations

// The algorithm we are using:
#include "../ptm_algs/p_orec_lazy.h"

// The templates we are using
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/orec_t.h"
#include "../common/p_alloc.h"
#include "../common/p_redolog.h"
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
typedef P_OrecLazy<
    OrecTable<NUM_STRIPES, OREC_COVERAGE, CounterTimesource>,
    P_RedoLog<2 << OREC_COVERAGE, true, true, ADR>,
    QuiesceEpochManager<MAX_THREADS>, ExpBackoffCM<BACKOFF_MIN, BACKOFF_MAX>,
    OptimizedStackFrameManager, BasicPersistentAllocationManager<ADR>, ADR>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class O, class R, class E, class C, class S, class A, class P>
typename P_OrecLazy<O, R, E, C, S, A, P>::Globals
    P_OrecLazy<O, R, E, C, S, A, P>::globals;

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