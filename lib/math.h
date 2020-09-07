#ifndef LIB_MATH_H
#define LIB_MATH_H

// Why is GCC like this...
// Supposedly a cross compiler would help, but that seems even more time
// consuming.

#include <stdint.h>

namespace lib {

inline uint64_t multiply(uint64_t lhs, uint64_t rhs) {
  uint64_t ret = 0;
  for (int i = 0; i < 64; i++) {
    ret += ((rhs >> i) & 1) ? (lhs << i) : 0;
  }

  return ret;
}

inline uint64_t divide(uint64_t lhs, uint64_t rhs) {
  uint64_t ret = 0;
  uint64_t acc = 0;
  for (int i = 63; i >= 0; i--) {
    acc = (acc << 1) | ((lhs >> i) & 1);
    if (rhs > acc) {
      ret = ret << 1;
    } else {
      ret = (ret << 1) | 1;
      acc -= rhs;
    }
  }

  return ret;
}

} // namespace lib

#endif
