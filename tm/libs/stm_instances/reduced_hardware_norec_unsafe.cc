/// Instantiate the ReducedHardwareNOrec algorithm with the following
/// configuration:
/// - 8 retries before HTM->POSTFIX, 8 retries before POSTFIX->STM
/// - 32-byte redo log
/// - Irrevocability and quiescence
/// - No contention management
/// - Support for dynamic stack frame optimizations
/// - Irrevocability after 128 mallocs, captured memory

// The algorithm we are using:
#include "../stm_algs/reduced_hardware_norec.h"

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
typedef ReducedHardwareNOrec<NUM_HTM_RETRIES, NUM_POSTFIX_RETRIES,
                             RedoLog_Nonatomic<32>, ValueLog_Nonatomic,
                             IrrevocQuiesceEpochManager<MAX_THREADS>, NoopCM,
                             OptimizedStackFrameManager,
                             BoundedAllocationManager<MALLOC_THRESHOLD, true>>
    TxThread;

/// Define TxThread::Globals.  Note that defining it this way ensures it is
/// initialized before any threads are created.
template <int RH, int RP, class R, class V, class E, class C, class S, class A>
typename ReducedHardwareNOrec<RH, RP, R, V, E, C, S, A>::Globals
    ReducedHardwareNOrec<RH, RP, R, V, E, C, S, A>::globals;

/// Initialize the API.  Be sure to use the HYBRID execute code
API_TM_DESCRIPTOR;
API_TM_MALLOC_FREE;
API_TM_MEMFUNCS_GENERIC;
API_TM_LOADFUNCS;
API_TM_STOREFUNCS;
API_TM_STATS_NOP;
API_TM_EXECUTE_HYBRID;
API_TM_CLONES_THREAD_UNSAFE;
API_TM_STACKFRAME_OPT;