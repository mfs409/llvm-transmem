/// Instantiate the P_CGL_Eager algorithm with the following configuration:
/// - std::mutex as the lock type
/// - Coarse-granularity undo logging (32 bytes)
/// - Basic nesting support
/// - Allocation with captured memory optimization
/// - ADR persistence domain
///
/// This is the "ideal persistent" implementation: the optimizations are
/// captured memory and coarse undo logging.

// The algorithm we are using:
#include "../ptm_algs/p_cgl_eager.h"

// The templates we are using
#include "../common/locks.h"
#include "../common/p_alloc.h"
#include "../common/p_coarse_undolog.h"
#include "../common/pmem_platform.h"
#include "../common/stackframe.h"

// The API generators
#include "../api/clone.h"
#include "../api/execute.h"
#include "../api/frame.h"
#include "../api/loadstore.h"
#include "../api/mem.h"
#include "../api/stats.h"

typedef P_CGL_Eager<wrapped_mutex, P_CoarseUndoLog<32, ADR>,
                    BasicStackFrameManager,
                    EnhancedPersistentAllocationManager<ADR>, ADR>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class L, class U, class S, class A, class P>
typename P_CGL_Eager<L, U, S, A, P>::Globals
    P_CGL_Eager<L, U, S, A, P>::globals;

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