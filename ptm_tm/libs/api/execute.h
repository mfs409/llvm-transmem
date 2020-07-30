/// execute.h implements the portion of the API related to launching
/// transactions and managing transaction descriptors.

#pragma once

#include <functional>
#include <setjmp.h>

#include "../../common/tm_defines.h"

/// Create helper methods that create a thread-local pointer to the TxThread,
/// and help a caller to get/construct one
#define API_TM_DESCRIPTOR                                                      \
  namespace {                                                                  \
  thread_local TxThread *self = nullptr;                                       \
  static TxThread *get_self() {                                                \
    if (__builtin_expect(self == nullptr, false)) {                            \
      self = new TxThread();                                                   \
    }                                                                          \
    return self;                                                               \
  }                                                                            \
  }

/// Create the API functions that are used to launch transactions.  These are
/// the versions for when we are not concerned about exceptions escaping from
/// transactions (that is, they don't use try/catch internally).
///
/// EXECUTE_C_INTERNAL is the call when the C compiler could find a clone.
/// EXECUTE_C is for when the compiler could not find a clone at compile time,
/// and needs to do the lookup at run time.  EXECUTE is for the C++ API, which
/// always uses a lambda, and thus doesn't need to worry about lookup.
#define API_TM_EXECUTE_NOEXCEPT                                                \
  extern "C" {                                                                 \
  void TM_EXECUTE_C_INTERNAL(void (*)(void *), void *args,                     \
                             void (*anno_func)(void *)) {                      \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    jmp_buf _jmpbuf;                                                           \
    setjmp(_jmpbuf);                                                           \
    self->beginTx(&_jmpbuf);                                                   \
    anno_func(args);                                                           \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_EXECUTE_C(void *, void (*func)(void *), void *args) {                \
    /* casting function ptr to void* is illegal, but it works on x86 */        \
    union {                                                                    \
      void *voidstar;                                                          \
      TM_C_FUNC cfunc;                                                         \
    } clone;                                                                   \
    clone.voidstar = get_clone((void *)func);                                  \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    jmp_buf _jmpbuf;                                                           \
    setjmp(_jmpbuf);                                                           \
    self->beginTx(&_jmpbuf);                                                   \
    /* If no clone, become irrevocable */                                      \
    if (clone.voidstar == nullptr) {                                           \
      self->becomeIrrevocable();                                               \
      func(args);                                                              \
    } else {                                                                   \
      clone.cfunc(args);                                                       \
    }                                                                          \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_EXECUTE(void *, std::function<void(TM_OPAQUE *)> func) {             \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    jmp_buf _jmpbuf;                                                           \
    setjmp(_jmpbuf);                                                           \
    self->beginTx(&_jmpbuf);                                                   \
    func((TM_OPAQUE *)0xCAFE);                                                 \
    self->commitTx();                                                          \
  }                                                                            \
  bool TM_RAII_BEGIN(jmp_buf &buffer) {                                        \
    TxThread *self = get_self();                                               \
    self->beginTx(&buffer);                                                    \
    return true;                                                               \
  }                                                                            \
  void TM_RAII_END() {                                                         \
    TxThread *self = get_self();                                               \
    self->commitTx();                                                          \
  }                                                                            \
  }

/// Create the API functions that are used to launch transactions.  These are
/// the versions for when we never run instrumented code (e.g., HTM, Mutex)
#define API_TM_EXECUTE_NOEXCEPT_NOINST                                         \
  extern "C" {                                                                 \
  void TM_EXECUTE_C_INTERNAL(void (*func)(void *), void *args,                 \
                             void (*)(void *)) {                               \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    self->beginTx();                                                           \
    func(args);                                                                \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_EXECUTE_C(void *, void (*func)(void *), void *args) {                \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    self->beginTx();                                                           \
    func(args);                                                                \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_EXECUTE(void *, std::function<void(TM_OPAQUE *)> func) {             \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    self->beginTx();                                                           \
    func(0);                                                                   \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_RAII_LITE_BEGIN() {                                                  \
    TxThread *self = get_self();                                               \
    self->beginTx();                                                           \
  }                                                                            \
  void TM_RAII_LITE_END() { self->commitTx(); }                                \
  bool TM_RAII_BEGIN(jmp_buf &) {                                              \
    TxThread *self = get_self();                                               \
    self->beginTx();                                                           \
    return false;                                                              \
  }                                                                            \
  void TM_RAII_END() {                                                         \
    TxThread *self = get_self();                                               \
    self->commitTx();                                                          \
  }                                                                            \
  }

/// Create the API functions that are used to launch hybrid transactions.  These
/// are the versions for when we are not concerned about exceptions escaping
/// from transactions (that is, they don't use try/catch internally), and we
/// aren't concerned about the TM aborting and needing a checkpoint to restore.
///
/// The main difference versus the code above is that begin() is split, based on
/// whether we are trying to begin in HTM (without a setjmp first) or not
#define API_TM_EXECUTE_HYBRID                                                  \
  extern "C" {                                                                 \
  void TM_EXECUTE_C_INTERNAL(void (*func)(void *), void *args,                 \
                             void (*anno_func)(void *)) {                      \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    if (self->beginHTM()) { /* Try to run as HTM */                            \
      func(nullptr);                                                           \
      self->commitTx();                                                        \
    } else {                                                                   \
      jmp_buf _jmpbuf;                                                         \
      setjmp(_jmpbuf);                                                         \
      self->beginSTM(&_jmpbuf);                                                \
      anno_func(args);                                                         \
      self->commitTx();                                                        \
    }                                                                          \
  }                                                                            \
  void TM_EXECUTE_C(void *, void (*func)(void *), void *args) {                \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    if (self->beginHTM()) { /* Try to run as HTM */                            \
      func(args);                                                              \
      self->commitTx();                                                        \
      return;                                                                  \
    }                                                                          \
    /* casting function ptr to void* is illegal, but it works on x86 */        \
    union {                                                                    \
      void *voidstar;                                                          \
      TM_C_FUNC cfunc;                                                         \
    } clone;                                                                   \
    clone.voidstar = get_clone((void *)func);                                  \
    jmp_buf _jmpbuf;                                                           \
    setjmp(_jmpbuf);                                                           \
    self->beginSTM(&_jmpbuf);                                                  \
    /* If no clone, become irrevocable */                                      \
    if (clone.voidstar == nullptr) {                                           \
      self->becomeIrrevocable();                                               \
      func(args);                                                              \
    } else {                                                                   \
      clone.cfunc(args);                                                       \
    }                                                                          \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_EXECUTE(void *, std::function<void(TM_OPAQUE *)> func) {             \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    if (self->beginHTM()) {                                                    \
      func(nullptr);                                                           \
      self->commitTx();                                                        \
    } else {                                                                   \
      jmp_buf _jmpbuf;                                                         \
      setjmp(_jmpbuf);                                                         \
      self->beginSTM(&_jmpbuf);                                                \
      func((TM_OPAQUE *)0xCAFE);                                               \
      self->commitTx();                                                        \
    }                                                                          \
  }                                                                            \
  bool TM_RAII_BEGIN(jmp_buf &buffer) {                                        \
    TxThread *self = get_self();                                               \
    if (self->beginHTM()) {                                                    \
      return false;                                                            \
    } else {                                                                   \
      self->beginSTM(&buffer);                                                 \
      return true;                                                             \
    }                                                                          \
  }                                                                            \
  void TM_RAII_END() {                                                         \
    TxThread *self = get_self();                                               \
    self->commitTx();                                                          \
  }                                                                            \
  }

/// Create the API functions that are used to launch transactions.  These are
/// the versions of EXECUTE_NOEXCEPT specialized for when transactions only
/// abort at commit time, and hence do not require setjmp/longjmp support.
///
/// NB: Irrevocability is not supported for these TMs, because we can't use
///     aborts to resolve two transactions attempting to be irrevocable
///     simultaneously
#define API_TM_EXECUTE_NOEXCEPT_LOOP_NOIRREVOC                                 \
  extern "C" {                                                                 \
  void TM_EXECUTE_C_INTERNAL(void (*)(void *), void *args,                     \
                             void (*anno_func)(void *)) {                      \
    TxThread *self = get_self();                                               \
    do {                                                                       \
      self->beginTx(&self);                                                    \
      anno_func(args);                                                         \
    } while (!self->commitTx());                                               \
  }                                                                            \
  void TM_EXECUTE_C(void *, void (*func)(void *), void *args) {                \
    /* casting function ptr to void* is illegal, but it works on x86 */        \
    union {                                                                    \
      void *voidstar;                                                          \
      TM_C_FUNC cfunc;                                                         \
    } clone;                                                                   \
    clone.voidstar = get_clone((void *)func);                                  \
    TxThread *self = get_self();                                               \
    do {                                                                       \
      self->beginTx(&self);                                                    \
      if (clone.voidstar == nullptr) {                                         \
        std::terminate();                                                      \
      } else {                                                                 \
        clone.cfunc(args);                                                     \
      }                                                                        \
    } while (!self->commitTx());                                               \
  }                                                                            \
  void TM_EXECUTE(void *, std::function<void(TM_OPAQUE *)> func) {             \
    TxThread *self = get_self();                                               \
    do {                                                                       \
      self->beginTx(&self);                                                    \
      func((TM_OPAQUE *)0xCAFE);                                               \
    } while (!self->commitTx());                                               \
  }                                                                            \
  bool TM_RAII_BEGIN(jmp_buf &) {                                              \
    TxThread *self = get_self();                                               \
    self->beginTx(&self);                                                      \
    return true;                                                               \
  }                                                                            \
  void TM_RAII_END() {                                                         \
    TxThread *self = get_self();                                               \
    self->commitTx();                                                          \
  }                                                                            \
  }

/// Create the API functions that are used to launch speculative PTM
/// transactions.  Since we are dealing with PTM, irrevocability is never an
/// option, and we must always take the instrumented code path.  Since we are
/// dealing with speculation, we require setjmp / longjmp.
///
/// Note that these are the versions for when we are not concerned about
/// exceptions escaping from transactions (that is, they don't use try/catch
/// internally).
#define API_TM_EXECUTE_NOEXCEPT_PTM                                            \
  extern "C" {                                                                 \
  void TM_EXECUTE_C_INTERNAL(void (*)(void *), void *args,                     \
                             void (*anno_func)(void *)) {                      \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    jmp_buf _jmpbuf;                                                           \
    setjmp(_jmpbuf);                                                           \
    self->beginTx(&_jmpbuf);                                                   \
    anno_func(args);                                                           \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_EXECUTE_C(void *, void (*func)(void *), void *args) {                \
    /* casting function ptr to void* is illegal, but it works on x86 */        \
    union {                                                                    \
      void *voidstar;                                                          \
      TM_C_FUNC cfunc;                                                         \
    } clone;                                                                   \
    clone.voidstar = get_clone((void *)func);                                  \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    jmp_buf _jmpbuf;                                                           \
    setjmp(_jmpbuf);                                                           \
    self->beginTx(&_jmpbuf);                                                   \
    /* If no clone, become irrevocable */                                      \
    if (clone.voidstar == nullptr) {                                           \
      std::terminate();                                                        \
    } else {                                                                   \
      clone.cfunc(args);                                                       \
    }                                                                          \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_EXECUTE(void *, std::function<void(TM_OPAQUE *)> func) {             \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    jmp_buf _jmpbuf;                                                           \
    setjmp(_jmpbuf);                                                           \
    self->beginTx(&_jmpbuf);                                                   \
    func((TM_OPAQUE *)0xCAFE);                                                 \
    self->commitTx();                                                          \
  }                                                                            \
  bool TM_RAII_BEGIN(jmp_buf &buffer) {                                        \
    TxThread *self = get_self();                                               \
    self->beginTx(&buffer);                                                    \
    return true;                                                               \
  }                                                                            \
  void TM_RAII_END() {                                                         \
    TxThread *self = get_self();                                               \
    self->commitTx();                                                          \
  }                                                                            \
  }

/// Create the API functions that are used to launch non-speculative PTM
/// transactions.  Since we are dealing with PTM, irrevocability is never an
/// option, and we must always take the instrumented code path.  However, we
/// don't need setjmp / longjmp.
///
/// Note that these are the versions for when we are not concerned about
/// exceptions escaping from transactions (that is, they don't use try/catch
/// internally).
#define API_TM_EXECUTE_NOEXCEPT_NOSPEC_PTM                                     \
  extern "C" {                                                                 \
  void TM_EXECUTE_C_INTERNAL(void (*)(void *), void *args,                     \
                             void (*anno_func)(void *)) {                      \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    self->beginTx(&self);                                                      \
    anno_func(args);                                                           \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_EXECUTE_C(void *, void (*func)(void *), void *args) {                \
    /* casting function ptr to void* is illegal, but it works on x86 */        \
    union {                                                                    \
      void *voidstar;                                                          \
      TM_C_FUNC cfunc;                                                         \
    } clone;                                                                   \
    clone.voidstar = get_clone((void *)func);                                  \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    self->beginTx(&self);                                                      \
    /* If no clone, become irrevocable */                                      \
    if (clone.voidstar == nullptr) {                                           \
      std::terminate();                                                        \
    }                                                                          \
    clone.cfunc(args);                                                         \
    self->commitTx();                                                          \
  }                                                                            \
  void TM_EXECUTE(void *, std::function<void(TM_OPAQUE *)> func) {             \
    /* get TxThread before making checkpoint, so it doesn't re-run on abort */ \
    TxThread *self = get_self();                                               \
    self->beginTx(&self);                                                      \
    func((TM_OPAQUE *)0xCAFE);                                                 \
    self->commitTx();                                                          \
  }                                                                            \
  bool TM_RAII_BEGIN(jmp_buf &) {                                              \
    TxThread *self = get_self();                                               \
    self->beginTx(&self);                                                      \
    return true;                                                               \
  }                                                                            \
  void TM_RAII_END() {                                                         \
    TxThread *self = get_self();                                               \
    self->commitTx();                                                          \
  }                                                                            \
  }
