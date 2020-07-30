/// Instantiate the TLRWEager algorithm with the following configuration:
/// - 2^20 bytelocks, supporting 56 threads max
/// - Irrevocability, but no quiescence after transactions
/// - Irrevocability for contention management
/// - Support for dynamic stack frame optimizations
/// - Irrevocability after 128 mallocs, and captured memory
/// - Reasonably good deadlock tuning
///
/// Please see the notes in the corresponding .h file, and in the constants
/// file.  TLRW is extremely sensitive to its tuning parameters, and it is hard
/// to find a configuration that works well all the time.  The numbers we are
/// using, coupled with irrevocability as a fallback contention manager, lead to
/// acceptable performance across all of STAMP.

// The algorithm we are using
#include "../stm_algs/tlrw_eager.h"

// The templates we are using
#include "../common/alloc.h"
#include "../common/bytelock_t.h"
#include "../common/cm.h"
#include "../common/epochs.h"
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
/// NB: TLRWEager can livelock with ExpBackoffCM CM on some STAMP benchmarks
typedef TLRWEager<
    BytelockTable<NUM_STRIPES, OREC_COVERAGE, BYTELOCK_MAX_THREADS>,
    IrrevocEpochManager<MAX_THREADS>, IrrevocCM<ABORTS_THRESHOLD>,
    OptimizedStackFrameManager,
    BoundedAllocationManager<MALLOC_THRESHOLD, true>, TLRW_READ_TRIES,
    TLRW_READ_SPINS, TLRW_WRITE_TRIES, TLRW_WRITE_SPINS>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class O, class E, class C, class S, class A, int RT, int RS, int WT,
          int WS>
typename TLRWEager<O, E, C, S, A, RT, RS, WT, WS>::Globals
    TLRWEager<O, E, C, S, A, RT, RS, WT, WS>::globals;

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