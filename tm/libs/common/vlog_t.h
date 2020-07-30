#pragma once

#include <atomic>
#include <cstdint>

/// vlog_t represents a single entry in the value log.  It consists of a
/// pointer, a value, and something to indicate the type that is logged
struct vlog_t {
  /// A lightweight enum to represent the type of element logged in this
  /// vlog_t. Valid values are 0/1/2/3/4/5/6/7,  for
  /// u8/u16/u32/u64/f32/f64/f80/pointer
  int type;

  /// A union storing the address, so that we can load/store through the
  /// address without casting
  union {
    uint8_t *u1;
    uint16_t *u2;
    uint32_t *u4;
    uint64_t *u8;
    float *f4;
    double *f8;
    long double *f16;
    void **p;
  } addr;

  /// A union storing the value that has been logged.  Again, we use a union
  /// to avoid casting
  union {
    uint8_t u1;
    uint16_t u2;
    uint32_t u4;
    uint64_t u8;
    float f4;
    double f8;
    long double f16;
    void *p;
  } val;

/// Initialize this vlog_t from a pointer by setting addr to the pointer and
/// then dereferencing the pointer to get the value for val.  We use
/// overloading to do this in a type-safe way for the 8 primitive types in our
/// STM, and a macro to avoid lots of boilerplate code
#define MAKE_VLOGT_FUNCS(TYPE, FIELD, TYPEID)                                  \
  void initFromAddr(TYPE *a) {                                                 \
    addr.FIELD = a;                                                            \
    val.FIELD = *a;                                                            \
    type = TYPEID;                                                             \
  }                                                                            \
  void make(TYPE *a, TYPE v) {                                                 \
    addr.FIELD = a;                                                            \
    val.FIELD = v;                                                             \
    type = TYPEID;                                                             \
  }
  MAKE_VLOGT_FUNCS(uint8_t, u1, 0);
  MAKE_VLOGT_FUNCS(uint16_t, u2, 1);
  MAKE_VLOGT_FUNCS(uint32_t, u4, 2);
  MAKE_VLOGT_FUNCS(uint64_t, u8, 3);
  MAKE_VLOGT_FUNCS(float, f4, 4);
  MAKE_VLOGT_FUNCS(double, f8, 5);
  MAKE_VLOGT_FUNCS(long double, f16, 6);
  MAKE_VLOGT_FUNCS(void *, p, 7);

  /// Restore the value at addr to the value stored in val
  void restoreValue() {
    switch (type) {
    case 0:
      *addr.u1 = val.u1;
      break;
    case 1:
      *addr.u2 = val.u2;
      break;
    case 2:
      *addr.u4 = val.u4;
      break;
    case 3:
      *addr.u8 = val.u8;
      break;
    case 4:
      *addr.f4 = val.f4;
      break;
    case 5:
      *addr.f8 = val.f8;
      break;
    case 6:
      *addr.f16 = val.f16;
      break;
    case 7:
      *addr.p = val.p;
      break;
    }
  }

  /// Check if the value in memory matches what we saved previously
  bool checkValue() {
    switch (type) {
    case 0:
      return *addr.u1 == val.u1;
    case 1:
      return *addr.u2 == val.u2;
    case 2:
      return *addr.u4 == val.u4;
    case 3:
      return *addr.u8 == val.u8;
    case 4:
      return *addr.f4 == val.f4;
    case 5:
      return *addr.f8 == val.f8;
    case 6:
      return *addr.f16 == val.f16;
    case 7:
      return *addr.p == val.p;
    }
    return false;
  }

  /// Check if the value in memory matches what we saved previously
  bool checkValue_atomic() {
    switch (type) {
    case 0:
      return reinterpret_cast<std::atomic<uint8_t> *>(addr.u1)->load(
                 std::memory_order_acquire) == val.u1;
    case 1:
      return reinterpret_cast<std::atomic<uint16_t> *>(addr.u2)->load(
                 std::memory_order_acquire) == val.u2;
    case 2:
      return reinterpret_cast<std::atomic<uint32_t> *>(addr.u4)->load(
                 std::memory_order_acquire) == val.u4;
    case 3:
      return reinterpret_cast<std::atomic<uint64_t> *>(addr.u8)->load(
                 std::memory_order_acquire) == val.u8;
    case 4:
      return reinterpret_cast<std::atomic<float> *>(addr.f4)->load(
                 std::memory_order_acquire) == val.f4;
    case 5:
      return reinterpret_cast<std::atomic<double> *>(addr.f8)->load(
                 std::memory_order_acquire) == val.f8;
    case 6:
      return reinterpret_cast<std::atomic<long double> *>(addr.f16)->load(
                 std::memory_order_acquire) == val.f16;
    case 7:
      return reinterpret_cast<std::atomic<void *> *>(addr.p)->load(
                 std::memory_order_acquire) == val.p;
    }
    return false;
  }
};
