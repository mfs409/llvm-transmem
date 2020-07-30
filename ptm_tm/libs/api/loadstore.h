/// loadstore.h implements the portion of the API required for transactional
/// loads and stores

#pragma once

/// Create the API functions that are substituted for memory loads from within a
/// transaction
#define API_TM_LOADFUNCS                                                       \
  extern "C" {                                                                 \
  uint8_t TM_LOAD_U1(uint8_t *ptr) { return get_self()->read<>(ptr); }         \
  uint16_t TM_LOAD_U2(uint16_t *ptr) { return get_self()->read<>(ptr); }       \
  uint32_t TM_LOAD_U4(uint32_t *ptr) { return get_self()->read<>(ptr); }       \
  uint64_t TM_LOAD_U8(uint64_t *ptr) { return get_self()->read<>(ptr); }       \
  float TM_LOAD_F(float *ptr) { return get_self()->read<>(ptr); }              \
  double TM_LOAD_D(double *ptr) { return get_self()->read<>(ptr); }            \
  long double TM_LOAD_LD(long double *ptr) { return get_self()->read<>(ptr); } \
  void *TM_LOAD_P(void **ptr) { return get_self()->read<>(ptr); }              \
  }

/// Create the API functions that are substituted for memory stores  from within
/// a transaction
#define API_TM_STOREFUNCS                                                      \
  extern "C" {                                                                 \
  void TM_STORE_U1(uint8_t val, uint8_t *ptr) { get_self()->write(ptr, val); } \
  void TM_STORE_U2(uint16_t val, uint16_t *ptr) {                              \
    get_self()->write(ptr, val);                                               \
  }                                                                            \
  void TM_STORE_U4(uint32_t val, uint32_t *ptr) {                              \
    get_self()->write(ptr, val);                                               \
  }                                                                            \
  void TM_STORE_U8(uint64_t val, uint64_t *ptr) {                              \
    get_self()->write(ptr, val);                                               \
  }                                                                            \
  void TM_STORE_F(float val, float *ptr) { get_self()->write(ptr, val); }      \
  void TM_STORE_D(double val, double *ptr) { get_self()->write(ptr, val); }    \
  void TM_STORE_LD(long double val, long double *ptr) {                        \
    get_self()->write(ptr, val);                                               \
  }                                                                            \
  void TM_STORE_P(void *val, void **ptr) { get_self()->write(ptr, val); }      \
  }