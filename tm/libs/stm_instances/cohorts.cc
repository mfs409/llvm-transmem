/// Instantiate the cohorts algorithm with the following configuration:
/// - Redo log with 32-byte granularity
/// - Support for dynamic stack frame optimizations
/// - Dynamic captured memory optimizations

// The algorithm we are using:
#include "../stm_algs/cohorts.h"

// The templates we are using
#include "../common/alloc.h"
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
typedef Cohorts<RedoLog_Nonatomic<32>, ValueLog_Nonatomic,
                OptimizedStackFrameManager, BasicAllocationManager<true>>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class R, class V, class S, class A>
typename Cohorts<R, V, S, A>::Globals Cohorts<R, V, S, A>::globals;

/// Initialize the API
API_TM_DESCRIPTOR;
API_TM_MALLOC_FREE;
API_TM_MEMFUNCS_GENERIC;
API_TM_LOADFUNCS;
API_TM_STOREFUNCS;
API_TM_STATS_NOP;
API_TM_EXECUTE_NOEXCEPT_LOOP_NOIRREVOC; // Needed for Cohorts
API_TM_CLONES_THREAD_UNSAFE;
API_TM_STACKFRAME_OPT;