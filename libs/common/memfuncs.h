/// memfuncs.h provides generic transactional versions of the standard memory
/// functions (memcpy, memset, memmove) from the C library's string.h header.
/// Any STM implementation should be able to call these, passing in a reference
/// to the caller's transaction descriptor in order to provide the template with
/// the ability to make appropriate read/write calls.

#pragma once

#include <cstring>

/// Basic transactional memcpy.  This does not optimize in any way for specific
/// TM algorithms (e.g., by doing a bulk orec acquire or a bulk undo logging).
/// Instead, it moves through the input and output regions, one chunk at a time,
/// performing an instrumented read from the former and an instrumented write to
/// the latter.
///
/// Note that this code *does* handle allignment and jagged start and end
/// points.
template <class T>
void *tx_basic_memcpy(void *dest, const void *src, size_t len, size_t align,
                      T &t) {
  if (t.isIrrevoc()) {
    memcpy(dest, src, len);
    return dest;
  }
  uintptr_t dp = (uintptr_t)dest, sp = (uintptr_t)src;
  uint8_t *d = (uint8_t *)dest, *s = (uint8_t *)src;
  // slowpath for incompatible alignment
  if (dp % 8 != sp % 8) {
    while (len--) {
      t.write(d, t.read(s)); // instrumented
      ++d;
      ++s;
    }
  }
  // compatible alignment
  else {
    // advance to next boundary
    while (len > 0 && dp % sizeof(uint64_t) != 0) {
      t.write(d, t.read(s)); // instrumented
      ++d;
      ++s;
      ++dp;
      --len;
    }
    if (len == -1)
      return dest;
    // aligned copying
    while (len >= sizeof(uint64_t)) {
      t.write((uint64_t *)d, t.read((uint64_t *)s)); // instrumented, wide
      d += sizeof(uint64_t);
      s += sizeof(uint64_t);
      len -= sizeof(uint64_t);
    }
    // handle unaligned end
    while (len--) {
      t.write(d, t.read(s)); // instrumented
      ++d;
      ++s;
    }
  }
  return dest;
}

/// Basic transactional memset.  This does not optimize in any way for specific
/// TM algorithms (e.g., by doing a bulk orec acquire).
/// Instead, it moves through the output region, one chunk at a time,
/// performing an instrumented write at each position.
///
/// Note that this code *does* handle allignment and jagged start and end
/// points.
template <class T>
void *tx_basic_memset(void *dest, int val, size_t len, T &t) {
  if (t.isIrrevoc()) {
    memset(dest, val, len);
    return dest;
  }
  uintptr_t dp = (uintptr_t)dest;
  uint8_t *d = (uint8_t *)dest, v = val;
  // build wide content
  uint64_t vv = v | v << 8 | v << 16 | v << 24;
  vv = vv << 32 | vv;
  // advance until aligned
  while (len > 0 && dp % sizeof(uint64_t) != 0) {
    t.write(d, v); // instrumented
    ++d;
    ++dp;
    --len;
  }
  if (len == -1)
    return dest;
  // aligned, wide writes
  while (len >= sizeof(uint64_t)) {
    t.write((uint64_t *)d, vv); // instrumented
    d += sizeof(uint64_t);
    len -= sizeof(uint64_t);
  }
  // end is unaligned?
  while (len--) {
    t.write(d, v); // instrumented
    ++d;
  }
  return dest;
}

/// Basic transactional memmove.  This does not optimize in any way for specific
/// TM algorithms (e.g., by doing a bulk orec acquire or bulk undo logging).
/// Instead, it moves through the input and output regions, copying one byte at
/// a time using instrumented reads and writes.
///
/// Note that this code *does* handle allignment and jagged start and end
/// points.
template <class T>
void *tx_basic_memmove(void *dest, const void *src, size_t len, T &t) {
  if (t.isIrrevoc()) {
    memmove(dest, src, len);
    return dest;
  }

  uint8_t *d = (uint8_t *)(dest), *s = (uint8_t *)(const_cast<void *>(src));
  if (d < s) {
    while (len--) {
      t.write(d, t.read(s));
      ++d;
      ++s;
    }
  } else {
    uint8_t *lasts = s + (len - 1), *lastd = d + (len - 1);
    while (len--) {
      t.write(lastd, t.read(lasts));
      --lastd;
      --lasts;
    }
  }
  return dest;
}