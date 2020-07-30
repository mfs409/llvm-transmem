/// clone.h implements the portion of the API required for managing the mappings
/// between nontransactional functions and their transactional clones.  It also
/// implements the portion of the API that is invoked when the compiler cannot
/// find a clone

#pragma once

#include <unordered_map>

namespace {
/// The function pointer translation table that TM manipulates in order to track
/// mappings from functions to their clones
///
/// WARNING: This version is not thread safe.  If a program were to have have
///       dynamic loading and unloading of shared objects during its execution,
///       this implementation would not be correct.  For the time being, we
///       assume that a programmer instantiating the TM API will be aware of the
///       problem, because of the use of a "THREAD_UNSAFE" macro (below).
///       Longer-term, we need to provide an epoch-based mechanism, where
///       threads wait to deregister a shared object.  Note that in the current
///       setting, as long as shared objects are only loaded and unloaded inside
///       of (irrevocable) transactions, there will be no races.
///
/// Note: Some TMs (CGL, HTM_GL) don't use clones, so we have to be able to
///       compile even when these functions are never called.  Hence the pragmas
///       to avoid compiler warnings.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
std::unordered_map<void *, void *> *clone_table;
#pragma clang diagnostic pop

/// Perform a lookup in the clone_table: if a clone is not found, return nullptr
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
void *get_clone(void *func) {
  auto found = clone_table->find(func);
  if (found != clone_table->end())
    return found->second;
  return nullptr;
}
#pragma clang diagnostic pop
} // namespace

/// Create the API functions that are used for interacting with the clone table.
/// TM_REG_CLONE is used when main begins, or a shared library is loading, to
/// add mappings to the clone table.  TM_UNSAFE is for when the compiler can't
/// find a clone, or can't run an instruction transactionally.  TM_TRANSLATE
/// calls are inserted by the compiler when it cannot find a clone statically.
#define API_TM_CLONES_THREAD_UNSAFE                                            \
  extern "C" {                                                                 \
  void TM_REG_CLONE(void *from, void *to) {                                    \
    if (clone_table == nullptr)                                                \
      clone_table = new std::unordered_map<void *, void *>();                  \
    clone_table->insert({from, to});                                           \
  }                                                                            \
  void TM_UNSAFE() { get_self()->becomeIrrevocable(); }                        \
  void *TM_TRANSLATE_CALL(void *func) {                                        \
    auto found = clone_table->find(func);                                      \
    if (found == clone_table->end()) {                                         \
      TM_UNSAFE();                                                             \
      return func;                                                             \
    } else {                                                                   \
      return found->second;                                                    \
    }                                                                          \
  }                                                                            \
  }

/// Create the API functions that are used for interacting with a nonexistent
/// clone table.  This is specifically for single-lock TMs, like MUTEX, which
/// are always irrevocable, or HTM-only TMs.
#define API_TM_CLONES_ALWAYS_IRREVOC                                           \
  extern "C" {                                                                 \
  void TM_REG_CLONE(void *from, void *to) {}                                   \
  /* This function will never be called, because we don't deal with clones */  \
  /* in this file or in the HTM/CGL version of TM_EXECUTE_C. */                \
  void TM_UNSAFE() { std::terminate(); }                                       \
  void *TM_TRANSLATE_CALL(void *func) { return func; }                         \
  }