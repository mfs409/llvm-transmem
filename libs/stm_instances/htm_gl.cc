/// Instantiate the HTM_GL algorithm with the following configuration:
/// - Basic nesting support
/// - Immediate allocation
/// - 8 HTM aborts before ralling back to the global lock

// The algorithm we are using:
#include "../stm_algs/htm_gl.h"

// The templates we are using
#include "../common/alloc.h"
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

typedef HTM_GL<BasicStackFrameManager, ImmediateAllocationManager,
               NUM_HTM_RETRIES>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <class S, class A, int R>
typename HTM_GL<S, A, R>::Globals HTM_GL<S, A, R>::globals;

API_TM_DESCRIPTOR;
API_TM_MALLOC_FREE;
API_TM_MEMFUNCS_GENERIC;
API_TM_LOADFUNCS;
API_TM_STOREFUNCS;
API_TM_STATS_NOP;
API_TM_EXECUTE_NOEXCEPT_NOINST; // No need for clones
API_TM_CLONES_THREAD_UNSAFE;
API_TM_STACKFRAME_NAIVE;
