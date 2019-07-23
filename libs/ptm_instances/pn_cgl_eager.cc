/// Instantiate the P_CGL_Eager algorithm with the following configuration:
/// - std::mutex as the lock type
/// - Basic nesting support
/// - Basic persistent undo logging
/// - Standard allocation
///
/// This is the "naive persistent" implementation: it doesn't have any
/// optimizations

// The algorithm we are using:
#include "../ptm_algs/p_cgl_eager.h"

// The templates we are using
#include "../common/locks.h"
#include "../common/p_alloc.h"
#include "../common/p_undolog.h"
#include "../common/stackframe.h"

// The API generators
#include "../api/clone.h"
#include "../api/execute.h"
#include "../api/frame.h"
#include "../api/loadstore.h"
#include "../api/mem.h"
#include "../api/stats.h"

typedef P_CGL_Eager<wrapped_mutex, P_UndoLog, BasicStackFrameManager,
                    BasicPersistentAllocationManager>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class L, class U, class S, class A>
typename P_CGL_Eager<L, U, S, A>::Globals P_CGL_Eager<L, U, S, A>::globals;

/// Initialize the API
API_TM_DESCRIPTOR;
API_TM_MALLOC_FREE;
API_TM_MEMFUNCS_GENERIC;
API_TM_LOADFUNCS;
API_TM_STOREFUNCS;
API_TM_STATS_NOP;
API_TM_EXECUTE_NOEXCEPT_NOSPEC_PTM;
API_TM_CLONES_THREAD_UNSAFE;
API_TM_STACKFRAME_OPT;