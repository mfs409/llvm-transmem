/// Instantiate the CGL algorithm with the following configuration:
/// - std::mutex as the lock type
/// - Basic nesting support
/// - Immediate allocation

// The algorithm we are using:
#include "../stm_algs/cgl.h"

// The templates we are using
#include "../common/alloc.h"
#include "../common/locks.h"
#include "../common/stackframe.h"

// The API generators
#include "../api/clone.h"
#include "../api/execute.h"
#include "../api/frame.h"
#include "../api/loadstore.h"
#include "../api/mem.h"
#include "../api/stats.h"

typedef CGL<wrapped_mutex, BasicStackFrameManager, ImmediateAllocationManager>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class L, class S, class A>
typename CGL<L, S, A>::Globals CGL<L, S, A>::globals;

/// Initialize the API.  There are no special API variants required by CGL,
/// either for correctness or performance.
API_TM_DESCRIPTOR;
API_TM_MALLOC_FREE;
API_TM_MEMFUNCS_GENERIC;
API_TM_LOADFUNCS;
API_TM_STOREFUNCS;
API_TM_STATS_NOP;
API_TM_EXECUTE_NOEXCEPT_NOINST; // No need for clones
API_TM_CLONES_THREAD_UNSAFE;
API_TM_STACKFRAME_NAIVE;
