#pragma once

#include <cstdint>

/// p_pipeline_log_t represents a single entry in an undo log.  It is used for
/// the TLRW pipeline optimization.
struct p_pipeline_log_t {
  /// A lightweight enum to represent the type of element logged in this
  /// p_pipeline_log_t. Valid values are 0/1/2/3/4/5/6/7,  for
  /// u8/u16/u32/u64/f32/f64/f80/pointer
  int type;

  /// A union storing the address, so that we can load/store through the address
  /// without casting
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

  /// A union storing the value that has been logged.  Again, we use a union to
  /// avoid casting
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

/// Initialize this p_pipeline_log_t from a pointer by setting addr to the
/// pointer and then dereferencing the pointer to get the value for val.  We use
/// overloading to do this in a type-safe way for the 8 primitive types in our
/// STM, and a macro to avoid lots of boilerplate code
#define MAKE_PPLT_FUNCS(TYPE, FIELD, TYPEID)                                   \
  void initFromAddr(TYPE *a) {                                                 \
    addr.FIELD = a;                                                            \
    val.FIELD = *a;                                                            \
    type = TYPEID;                                                             \
  }                                                                            \
  void initFromVal(TYPE *a, TYPE v) {                                          \
    addr.FIELD = a;                                                            \
    val.FIELD = v;                                                             \
    type = TYPEID;                                                             \
  }
  MAKE_PPLT_FUNCS(uint8_t, u1, 0);
  MAKE_PPLT_FUNCS(uint16_t, u2, 1);
  MAKE_PPLT_FUNCS(uint32_t, u4, 2);
  MAKE_PPLT_FUNCS(uint64_t, u8, 3);
  MAKE_PPLT_FUNCS(float, f4, 4);
  MAKE_PPLT_FUNCS(double, f8, 5);
  MAKE_PPLT_FUNCS(long double, f16, 6);
  MAKE_PPLT_FUNCS(void *, p, 7);

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
};