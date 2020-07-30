// This file defines the programmer API for our transactional memory
// implementation.
//
// Broadly, there are three APIs:
//
// - Lambda API: transactions are lambdas run by an executor
//
// - C (legacy) API: transactions are functions
//
// - RAII API: transactions are lexically-scoped regions delineated by a stack
//   object
//
// The lambda API is easier to implement in the compiler plugin.  However, the
// RAII API has less boundary overhead, because it does not involve lambdas.
//
// The APIs also have a "light" variant, in which there is no compiler
// instrumentation.  This strategy only supports two TM libraries: CGL (all
// transactions protected by the same global reentrant lock) and HTM_GL (use
// Intel TSX, fall back to a lock).  The "light" variant of the RAII
// implementation needs different macros than the full version.  The C and
// lambda APIs do not need anything special from this file in order to use
// "lite" mode.
//
// All of the APIs benefit from the following common calls:
//
// - TX_SAFE:                 Indicate that a function is always safe to call
//                            from transactions
//
// - TX_PURE:                 Indicate that a function does not need to be
//                            instrumented: it is safe as-is to call from a
//                            transaction.
//
// - TX_RENAME:               Provide a different name to use for the cloned
//                            version of the corresponding function.  This is
//                            useful when providing instrumentable versions of
//                            standard library functions.
//
// - TX_PRIVATE_STACK_REGION: Assist the plugin in avoiding instrumentation for
//                            locations that are local to a transaction but not
//                            within the transactional portion of the stack.
//
// - TX_COMMIT_HANDLER:       Specify code to run when a transaction commits
//
// - TX_CTOR:                 Instruct the plugin to instrument a constructor

#pragma once

#include <functional>
#include <setjmp.h>

#include "tm_defines.h"

//
// Below is the portion of the API that is common to all APIs
//

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

// Public:   TX_RENAME allows the programmer to indicate that a function should
//           (a) be instrumented, (b) not be cloned, and (c) be re-named, so it
//           can serve as the TX_SAFE clone for a function lacking a definition.
//
// Internal: The generated name will include a prefix, so that the plugin can
//           figure out names correctly, without front-end support.  This means
//           that the programmer must manually mangle names, when C++ linkage
//           is used
#define TX_RENAME(newname) TM_RENAME(newname)

// Public:   TX_PRIVATE_STACK_REGION allows the programmer to indicate that some
//           suffix of the non-transactional stack frame does not require
//           instrumentation
//
// Internal: N/A
#define TX_PRIVATE_STACK_REGION(ptr) TM_SET_STACKFRAME(ptr)

// Public:   TX_COMMIT_HANDLER provides support for oncommit handlers
//
// Internal:
#define TX_COMMIT_HANDLER(func, params) TM_COMMIT_HANDLER((func), (params))

// Public:   TX_CTOR helps a programmer indicate to the plugin that a
//           constructor may be called by transactions in another translation
//           unit.
//
// Internal: N/A
#define TX_CTOR TM_CTOR

// The common part of the API requires these declarations
extern "C" {
void TM_CTOR(void);
void TM_SET_STACKFRAME(void *);
void TM_COMMIT_HANDLER(TM_C_FUNC, void *args);
}

//
// The mechanism used by the C API for launching a transaction is a single
// function, TX_BEGIN_C(), which takes as a parameter a function pointer, and
// attempts to execute that function as a transaction.  It also takes a void*,
// which it passes to the function it attempts to execute transactionally.
//

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

// The C API requires these delcarations
extern "C" {
void TM_EXECUTE_C(void *flags, TM_C_FUNC, void *args);
}

//
// The mechanism used by the Lambda API for launching a transaction is a
// function that takes a lambda as a parameter.  To make it feel more like a
// lexically-scoped transaction, we make the beginning and end of the function
// call (and lambda captures) into macros.  This also lets us add a hidden
// parameter to the lambda, which makes our instrumentation much easier.
//

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

// The lambda API requires these declarations
extern "C" {
void TM_EXECUTE(void *flags, std::function<void(TM_OPAQUE *)> func);
}

//
// The mechanism used by the RAII API for launching a transaction is a
// stack-constructed object that initializes the transaction upon construction,
// and commits it upon destruction.  To get it all to work correctly, we need to
// setjmp /before/ we construct the object, so we hide the setjmp and
// constructor in a macro.
//

// Public:   To execute a region of C++ code as a transaction, checkpoint the
//           thread's architectural state with a setjmp, then pass the jmp_buf
//           to the constructor for the transaction object.  The programmer
//           needs to be sure to do the scoping correctly: this macro should be
//           the first code after an open brace.
//
// Internal: The control flow subgraph in which the RAII object is live will be
//           transformed.
#if !defined(TM_USE_RAII_LITE)
#define TX_RAII                                                                \
  jmp_buf TM_RAII_JMPBUF;                                                      \
  setjmp(TM_RAII_JMPBUF);                                                      \
  bool TM_RAII_TXSTATE;                                                        \
  TM_RAII tx(TM_RAII_TXSTATE, TM_RAII_JMPBUF)

// The RAII API requires these function declarations
extern "C" {
bool TM_RAII_BEGIN(jmp_buf &);
void TM_RAII_END();
}

// Public:   The RAII struct that we use to mark a transactional region
//
// Internal: N/A
struct TM_RAII {
  TM_RAII(bool &take_inst, jmp_buf &buffer) {
    take_inst = TM_RAII_BEGIN(buffer);
  }
  ~TM_RAII() { TM_RAII_END(); }
};

#else
#define TX_RAII                                                                \
  bool TM_RAII_TXSTATE;                                                        \
  TM_RAII tx(TM_RAII_TXSTATE)

// The RAII_LITE API requires these function declarations
extern "C" {
bool TM_RAII_LITE_BEGIN();
void TM_RAII_LITE_END();
}

// Public:   The RAII struct that we use to mark a transactional region
//
// Internal: N/A
struct TM_RAII {
  TM_RAII(bool &take_inst) { take_inst = TM_RAII_LITE_BEGIN(); }
  ~TM_RAII() { TM_RAII_LITE_END(); }
};
#endif