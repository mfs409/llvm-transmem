// In order to ensure that our plugin, our libraries, and any programs we write
// all make use of the same common names for everything, we put all names into
// this file.
//
// Note that this file is included in the llvm pass, and in the implementation
// of each TM library.  It is also included in every application that uses TM,
// indirectly, through the tm_api.h header.

#pragma once

//
// These definitions are used by both the plugin and programmers who write
// application code
//

// The annotation that gets placed on functions that need to be cloned by TM
#define TM_ANNOTATE tm_function
#define TM_ANNOTATE_STR "tm_function"

// The annotation that gets placed on pure functions
#define TM_PURE tm_pure
#define TM_PURE_STR "tm_pure"

// The function that executes an instrumented function via the C API
#define TM_EXECUTE_C tm_execute_c
#define TM_EXECUTE_C_STR "tm_execute_c"

// The function that executes an instrumented region via the C++ "lambda" API
#define TM_EXECUTE tm_execute
#define TM_EXECUTE_STR "tm_execute"

// The RAII class that executes an instrumented region via the C++ "raii" API
#define TM_RAII tm_raii
#define TM_RAII_STR "tm_raii"

// The RAII class that executes an uninstrumented region via the C++ "raii_lite"
// API
#define TM_RAII_LITE tm_raii
#define TM_RAII_LITE_STR "tm_raii"

// The helper function to find a constructor
#define TM_CTOR tm_ctor
#define TM_CTOR_STR "tm_ctor"

// The function that indicate a transaction is a read-only transaction.
#define TM_SET_STACKFRAME tm_set_stackframe
#define TM_SET_STACKFRAME_STR "tm_set_stackframe"

// The function that provides support for oncommit handlers
#define TM_COMMIT_HANDLER tm_commit_handler
#define TM_COMMIT_HANDLER_STR "tm_commit_handler"

// The annotation for replacing function names for programmer-provided
// alternatives to functions with missing definitions
#define TM_RENAME_STR "tm_rename_"
#define TM_RENAME_1(newname) TM_RENAME_STR #newname
#define TM_RENAME(newname) __attribute__((annotate(TM_RENAME_1(newname))))

// The type of functions that get translated by TM_TRANSLATE_CALL.  Also the
// type of functions passed to commit handlers
#define TM_C_FUNC tm_c_func
typedef void (*TM_C_FUNC)(void *);

//
// These definitions are used by both the plugin and library code
//

// The library must implement this function, and the plugin will replace calls
// to TM_EXECUTE_C with calls to TM_EXECUTE_C_INTERNAL in order to run
// functions as instrumented code
#define TM_EXECUTE_C_INTERNAL tm_execute_c_internal
#define TM_EXECUTE_C_INTERNAL_STR "tm_execute_c_internal"

// The library will implement these memory access functions, and the plugin
// will replace loads and stores with calls to these functions.
#define TM_LOAD_U1 tm_load_u1
#define TM_LOAD_U1_STR "tm_load_u1"
#define TM_STORE_U1 tm_store_u1
#define TM_STORE_U1_STR "tm_store_u1"
#define TM_LOAD_U2 tm_load_u2
#define TM_LOAD_U2_STR "tm_load_u2"
#define TM_STORE_U2 tm_store_u2
#define TM_STORE_U2_STR "tm_store_u2"
#define TM_LOAD_U4 tm_load_u4
#define TM_LOAD_U4_STR "tm_load_u4"
#define TM_STORE_U4 tm_store_u4
#define TM_STORE_U4_STR "tm_store_u4"
#define TM_LOAD_U8 tm_load_u8
#define TM_LOAD_U8_STR "tm_load_u8"
#define TM_STORE_U8 tm_store_u8
#define TM_STORE_U8_STR "tm_store_u8"
#define TM_LOAD_F tm_load_f
#define TM_LOAD_F_STR "tm_load_f"
#define TM_STORE_F tm_store_f
#define TM_STORE_F_STR "tm_store_f"
#define TM_LOAD_D tm_load_d
#define TM_LOAD_D_STR "tm_load_d"
#define TM_STORE_D tm_store_d
#define TM_STORE_D_STR "tm_store_d"
#define TM_LOAD_LD tm_load_ld
#define TM_LOAD_LD_STR "tm_load_ld"
#define TM_STORE_LD tm_store_ld
#define TM_STORE_LD_STR "tm_store_ld"
#define TM_LOAD_P tm_load_p
#define TM_LOAD_P_STR "tm_load_p"
#define TM_STORE_P tm_store_p
#define TM_STORE_P_STR "tm_store_p"

// The library will implement these functions, and the plugin will
// replace calls to malloc and free and memory intrinsic with calls to these
// functions
#define TM_MALLOC tm_malloc
#define TM_MALLOC_STR "tm_malloc"
#define TM_ALIGNED_ALLOC tm_aligned_alloc
#define TM_ALIGNED_ALLOC_STR "tm_aligned_alloc"
#define TM_FREE tm_free
#define TM_FREE_STR "tm_free"
#define TM_MEMCPY tm_memcpy
#define TM_MEMCPY_STR "tm_memcpy"
#define TM_MEMSET tm_memset
#define TM_MEMSET_STR "tm_memset"
#define TM_MEMMOVE tm_memmove
#define TM_MEMMOVE_STR "tm_memmove"

// The library will implement these helper functions, and the plugin will
// add calls to these, as appropriate
#define TM_UNSAFE tm_unsafe
#define TM_UNSAFE_STR "tm_unsafe"
#define TM_TRANSLATE_CALL tm_translate_call
#define TM_TRANSLATE_CALL_STR "tm_translate_call"
#define TM_RAII_CTOR "_ZN7" TM_RAII_STR "C2ERbRA1_13__jmp_buf_tag"
#define TM_RAII_DTOR "_ZN7" TM_RAII_STR "D2Ev"

// The library will need to work with these functions
#define TM_SETJUMP_NAME "_setjmp"
#define TM_LAMBDA_BASE_NAME "_ZNSt14_Function_baseD2Ev"

//
// These definitions are only used by the plugin
//
#define TM_PREFIX_STR "tm_"

// An opaque pointer type that can be used to find lambdas that need to be added
// to the root set during plugin execution.  We hide this from the programmer
// via another macro.
#define TM_OPAQUE tm_opaque
#define TM_OPAQUE_STR "tm_opaque"
struct TM_OPAQUE;

// The API function for registering a clone
#define TM_REG_CLONE tm_register_clone
#define TM_REG_CLONE_STR "tm_register_clone"

// The name of the function that does clone registration
#define TM_STATIC_INITIALIZER_STR "tm_initialization"

//
// These definitions are only used by application code, and are not visible to
// the programmer
//

// The name of a setjmp buffer created as part of the RAII API
#define TM_RAII_JMPBUF tm_raii_jmp_buf

// The name of a bool that tracks the instrumentation state in the RAII API
#define TM_RAII_TXSTATE tm_raii_should_run_instrumented

// The function called by the RAII API to start a transaction
#define TM_RAII_BEGIN tm_raii_begin

// The function called by the RAII API to end a transaction
#define TM_RAII_END tm_raii_end

// The function called by the RAII_LITE API to start a transaction
#define TM_RAII_LITE_BEGIN tm_raii_lite_begin

// The function called by the RAII_LITE API to end a transaction
#define TM_RAII_LITE_END tm_raii_lite_end