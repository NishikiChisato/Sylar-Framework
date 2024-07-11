#ifndef __SYLAR_ENDIAN_H__
#define __SYLAR_ENDIAN_H__

#include <byteswap.h>
#include <stdint.h>

namespace Sylar {

#define SYLAR_LITTLE_ENDIAN 1
#define SYLAR_BIG_ENDIAN 2

template <typename T> T ByteSwap(T val) {
  if (sizeof(T) == sizeof(uint16_t)) {
    return bswap_16(val);
  } else if (sizeof(T) == sizeof(uint32_t)) {
    return bswap_32(val);
  } else if (sizeof(T) == sizeof(uint64_t)) {
    return bswap_64(val);
  }
  return 0;
}

#if BYTE_ORDER == BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#endif

#if SYLAR_BYTE_ORDER == SYLAR_LITTLE_ENDIAN

template <typename T> T SwapToLittle(T val) { return val; }

template <typename T> T SwapToBig(T val) { return ByteSwap(val); }

#elif SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN

template <typename T> T SwapToLittle(T val) { return ByteSwap(val); }

template <typename T> T SwapToBig(T val) { return val; }

#endif

} // namespace Sylar

#endif