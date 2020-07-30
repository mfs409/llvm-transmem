/// Instantiate the TMLLazy algorithm with the following configuration:
/// - Redolog with 32 granularity
/// - Irrevocability
/// - No contention management
/// - Support for dynamic stack frame optimizations
/// - Irrevocability after 128 mallocs

// The algorithm we are using
#include "../stm_algs/tml_lazy.h"

// The templates we are using
#include "../common/alloc.h"
#include "../common/cm.h"
#include "../common/epochs.h"
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
typedef TMLLazy<RedoLog_Nonatomic<32>, IrrevocEpochManager<MAX_THREADS>, NoopCM,
                OptimizedStackFrameManager,
                BoundedAllocationManager<MALLOC_THRESHOLD, true>>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class R, class E, class C, class S, class A>
typename TMLLazy<R, E, C, S, A>::Globals TMLLazy<R, E, C, S, A>::globals;

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
