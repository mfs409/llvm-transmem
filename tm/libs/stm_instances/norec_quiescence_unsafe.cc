/// Instantiate the NOrec algorithm with the following configuration:
/// - Redo log with 32-byte granularity
/// - Irrevocability and Quiescence
/// - No contention management
/// - Support for dynamic stack frame optimizations
/// - Irrevocability after 128 mallocs
/// - Dynamic captured memory optimizations

// The algorithm we are using:
#include "../stm_algs/norec.h"

// The templates we are using
#include "../common/alloc.h"
#include "../common/cm.h"
#include "../common/epochs.h"
#include "../common/redolog_nonatomic.h"
#include "../common/stackframe.h"
#include "../common/valuelog_nonatomic.h"

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
typedef NOrec<RedoLog_Nonatomic<32>, ValueLog_Nonatomic,
              IrrevocQuiesceEpochManager<MAX_THREADS>, NoopCM,
              OptimizedStackFrameManager,
              BoundedAllocationManager<MALLOC_THRESHOLD, true>>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class R, class V, class E, class C, class S, class A>
typename NOrec<R, V, E, C, S, A>::Globals NOrec<R, V, E, C, S, A>::globals;

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