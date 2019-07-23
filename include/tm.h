// This file defines the programmer API for our transactional memory
// implementation
//
// |------------+------------------|-------------------------------------------|
// |  keyword   | TMTS Equivalent  | Purpose
// |------------+------------------|-------------------------------------------|
// | TX_SAFE    | transaction_safe | indicate that a function is always safe   |
// |            |                  | to call from transactions                 |
// | TX_BEGIN_C | N/A              | execute a function as a new transactional |
// |            |                  | context                                   |
// | TX_BEGIN   | atomic {         | delineate the beginning of a lexically-   |
// |            |                  | scoped transaction                        |
// | TX_END     | }                | delineate the end of a lexically-scoped   |
// |            |                  | transaction                               |
// | TX_PURE    | transaction_pure | indicate that a function does not need to |
// |            |                  | be instrumented -- it is safe as-is to    |
// |            |                  | call from a transaction.                  |
// | TX_RENAME  | transaction_wrap | provide a different name to use for the   |
// |            |                  | cloned version of the corresponding       |
// |            |                  | function
// |------------+------------------|-------------------------------------------|
#pragma once

#include <functional>
#include <pthread.h>

#include "tm_api.h"

// Public:   Any function that is called from a transaction should be marked as
//           TX_SAFE
//
// Internal: TX_SAFE translates to the basic TM annotation, so that the
//           plugin knows include the function in the root set of code regions
//           that may be run transactionally
#define TX_SAFE __attribute__((annotate(TM_ANNOTATE_STR)))

// Public:   A function marked TX_PURE does not need instrumentation in order to
//           be called from a transaction
//
//           NB: This annotation is only applied to function definitions, not
//               declarations
//
// Internal: A function definition marked in this way will not be cloned or
//           added to the root set.  It will still be called direclty, not via
//           TM_TRANSLATE_CALL.
#define TX_PURE __attribute__((annotate(TM_PURE_STR)))

// Public:   To execute a TX_SAFE function as a transaction, the function must
//           have the signature void(*)(void*).  To launch it, pass the
//           function name and parameter to TM system via this command.  This
//           is the "C" interface to our TM interface
//
// Internal: TX_BEGIN_C forwards to TM_EXECUTE_C, with a first parameter that
//           indicates the desire to run the region using TM.  This call will
//           be transformed by the plugin into a call to an internal
//           (4-parameter) function for launching code with TM
#define TX_BEGIN_C(flags, func, params) TM_EXECUTE_C(flags, (func), (params))

// Public:   To execute a region of C++ code as a transaction, delineate it
//           with 'TX_BEGIN {' and '} TX_END;'.  Be sure not to put a semicolon
//           after TX_BEGIN.
//
//           NB: the region will become a lambda.  Be careful about clever
//               control flows, such as breaks, gotos, and returns.
//
// Example:
//           TX_BEGIN {
//               x++;
//           } TX_END;
//
// Internal: The body of the lambda will be transformed so that it has both
//           instrumented and uninstrumented code paths
#define TX_BEGIN TM_EXECUTE(nullptr, [&](TM_OPAQUE*)
#define TX_END   )

// Public:   TX_RENAME allows the programmer to indicate that a function should
//           (a) be instrumented, (b) not be cloned, and (c) be re-named, so it
//           can serve as the TX_SAFE clone for a function lacking a definition.
//
// Internal: The generated name will include a prefix, so that the plugin can
//           figure out names correctly, without front-end support.  This means
//           that the programmer must manually mangle names, when C++ linkage
//           is used
#define TX_RENAME(newname) TM_RENAME(newname)

// Public:   TX_PRIVATE_STACK_REGION allows the programmer to indicate that a
//           transaction is a read-only transaction.
//
// Internal:

#define TX_PRIVATE_STACK_REGION(ptr) TM_SET_STACKFRAME(ptr)

// Public:   TX_COMMIT_HANDLER provides support for oncommit handlers
//
// Internal:

#define TX_COMMIT_HANDLER(func, params) TM_COMMIT_HANDLER((func), (params))

// In order for programs to correctly compile, they must believe that there
// are valid TM_EXECUTE_C and TM_EXECUTE functions for them to execute.
// These declarations suffice to satisfy the compiler.
extern "C" {
void TM_EXECUTE_C(void *flags, TM_C_FUNC, void *args);
void TM_EXECUTE(void *flags, std::function<void(TM_OPAQUE *)> func);
void TM_CTOR(void);
void TM_SET_STACKFRAME(void *);
void TM_COMMIT_HANDLER(TM_C_FUNC, void *args);
}
