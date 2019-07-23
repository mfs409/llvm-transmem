/// Instantiate the OrecMixed algorithm with the following configuration:
/// - An orec table with 2^20 entries and a coverage of 16 bytes
/// - Redo log with orec granularity
/// - Irrevocability and Quiescence
/// - Hourglass contention management, since we don't have irrevocability
/// - Support for dynamic stack frame optimizations
/// - Irrevocability after 128 mallocs, and dynamic captured memory support

// The algorithm we are using:
#include "../stm_algs/orec_mixed.h"

// The templates we are using
#include "../common/alloc.h"
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/orec_t.h"
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
typedef OrecMixed<
    OrecTable<NUM_STRIPES, OREC_COVERAGE>, RedoLog<2 << OREC_COVERAGE>,
    IrrevocQuiesceEpochManager<MAX_THREADS>,
    HourglassCM<ABORTS_THRESHOLD>,
    OptimizedStackFrameManager,
    BoundedAllocationManager<MALLOC_THRESHOLD, true>>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class O, class R, class E, class C, class S, class A>
typename OrecMixed<O, R, E, C, S, A>::Globals
    OrecMixed<O, R, E, C, S, A>::globals;

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
