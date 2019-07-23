#pragma once

#include "llvm/IR/Module.h"

/// signatures is a convenience class that provides references to pre-built LLVM
/// Type* and Function* representations of the types and functions we use
/// throughout the plugin
class signatures {
public:
  /// An enum to avoid unnecessary hard-coding of array indices when looking up
  /// load and store functions
  enum VarTypes {
    U1 = 0, // 8-bit integer
    U2 = 1, // 16-bit integer
    U4 = 2, // 32-bit integer
    U8 = 3, // 64-bit integer
    F = 4,  // 32-bit float
    D = 5,  // 64-bit float
    LD = 6, // 80 (x86) or 128-bit (other architectures) float
    P = 7,  // pointer (whatever size that may be; only tested with 64-bit)
    VT_COUNT = 8, // # entries in this enum
  };

  /// An enum to avoid unnecessary hard-coding of array indices when looking up
  /// other instrumented functions
  enum FuncNames {
    CAPI = 0,          // C API execute transaction function
    MALLOC = 1,        // malloc
    ALIGNED_ALLOC = 2, // aligned_alloc
    FREE = 3,          // free
    MEMCPY = 4,        // memcpy
    MEMSET = 5,        // memset
    MEMMOVE = 6,       // memmove
    TRANSLATE = 7,     // tm_translate_call
    UNSAFE = 8,        // tm_unsafe
    FN_COUNT = 9,      // # entries in this enum
  };

  /// An enum to avoid unnecessary hard-coding of array indices when looking up
  /// LLVM Type*s to represent frequently-used types
  enum TypeNames {
    I8 = 0,      // 8-bit integer
    I16 = 1,     // 16-bit integer
    I32 = 2,     // 32-bit integer
    I64 = 3,     // 64-bit integer
    F32 = 4,     // 32-bit float
    F64 = 5,     // 64-bit float
    F128 = 6,    // 80 (x86) or 128-bit (other architectures) float
    I8P = 7,     // pointer to 8-bit integer
    I16P = 8,    // pointer to 16-bit integer
    I32P = 9,    // pointer to 32-bit integer
    I64P = 10,   // pointer to 64-bit integer
    F32P = 11,   // pointer to 32-bit float
    F64P = 12,   // pointer to 64-bit float
    F128P = 13,  // pointer to 80 (x86) or 128-bit (other architectures) float
    VOID = 14,   // void type
    OPAQUE = 15, // void*
    PTR_OPAQUE = 16, // void**
    TN_COUNT = 17,   // # entries in this enum
  };

private:
  /// Helper function to convert from llvm types to VarTypes
  static int type_to_vartype(llvm::Type *t);

  /// Signatures of the tm_load_* functions
  llvm::Function *loads[VarTypes::VT_COUNT];

  /// Signatures of the tm_store_* functions
  llvm::Function *stores[VarTypes::VT_COUNT];

  /// Signatures of the other tm instrumentation functions
  llvm::Function *funcs[FuncNames::FN_COUNT];

  /// Commonly-used types
  llvm::Type *types[TypeNames::TN_COUNT];

public:
  /// Initialize the signatures object by creating Type* objects that can be
  /// reused throughout the plugin, and by inserting Function*
  /// declarations into the Module for any TM library function we might ever
  /// call.
  void init(llvm::Module &M);

  /// Get the appropriate tm_load_* function for the provided type
  llvm::Function *get_load(llvm::Type *type) {
    if (type_to_vartype(type) != -1) {
      return loads[type_to_vartype(type)];
    } else {
      return nullptr;
    }
  }

  /// Get the appropriate tm_store_* function for the provided type
  llvm::Function *get_store(llvm::Type *type) {
    if (type_to_vartype(type) != -1) {
      return stores[type_to_vartype(type)];
    } else {
      return nullptr;
    }
  }

  /// Get an instrumented function
  llvm::Function *get_func(FuncNames f) { return funcs[f]; }

  /// Get a type
  llvm::Type *get_type(TypeNames t) { return types[t]; }
};
