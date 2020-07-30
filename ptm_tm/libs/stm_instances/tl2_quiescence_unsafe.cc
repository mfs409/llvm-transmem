/// Instantiate the OrecLazy algorithm with the following configuration:
/// - An orec table with 2^20 entries and a coverage of 16 bytes
/// - A redo log sized according to orec granularity
/// - Irrevocability and Quiescence
/// - Exponential Backoff for contention management
/// - Support for dynamic stack frame optimizations
/// - Irrevocability after 128 mallocs, and dynamic captured memory support
/// - No single fence optimizations

// The algorithm we are using:
#include "../stm_algs/tl2.h"

// The templates we are using
#include "../common/alloc.h"
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/orec_t.h"
#include "../common/redolog_nonatomic.h"
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
typedef TL2<OrecTable<NUM_STRIPES, OREC_COVERAGE, CounterTimesource>,
            RedoLog_Nonatomic<2 << OREC_COVERAGE>,
            IrrevocQuiesceEpochManager<MAX_THREADS>,
            ExpBackoffCM<BACKOFF_MIN, BACKOFF_MAX>, OptimizedStackFrameManager,
            BoundedAllocationManager<MALLOC_THRESHOLD, true>, false>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class O, class R, class E, class C, class S, class A, bool SFO>
typename TL2<O, R, E, C, S, A, SFO>::Globals
    TL2<O, R, E, C, S, A, SFO>::globals;

/// Initialize the API
API_TM_DESCRIPTOR;
API_TM_MALLOC_FREE;
API_TM_MEMFUNCS_GENERIC;
API_TM_LOADFUNCS;
API_TM_STOREFUNCS;
API_TM_STATS_NOP;
API_TM_EXECUTE_NOEXCEPT;
API_TM_CLONES_THREAD_UNSAFE;
API_TM_STACKFRAME_OPT;