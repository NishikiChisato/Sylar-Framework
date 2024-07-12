#include "include/stream.hh"

namespace Sylar {

size_t Stream::ReadFixedSize(void *buffer, size_t length) {
  size_t offset = 0;
  size_t left = length;
  while (left > 0) {
    size_t n = Read((char *)buffer + offset, left);
    if (n <= 0) {
      return n;
    }
    left -= n;
    offset += n;
  }
  return length;
}

size_t Stream::ReadFixedSize(ByteArray::ptr ba, size_t length) {
  size_t offset = 0;
  size_t left = length;
  while (left > 0) {
    size_t n = Read(ba, left);
    if (n <= 0) {
      return n;
    }
    left -= n;
    offset += n;
  }
  return length;
}

size_t Stream::WriteFixedSize(const void *buffer, size_t length) {
  size_t offset = 0;
  size_t left = length;
  while (left > 0) {
    size_t n = Write((char *)buffer + offset, left);
    if (n <= 0) {
      return n;
    }
    left -= n;
    offset += n;
  }
  return length;
}

size_t Stream::WriteFixedSize(ByteArray::ptr ba, size_t length) {
  size_t offset = 0;
  size_t left = length;
  while (left > 0) {
    size_t n = Write(ba, left);
    if (n <= 0) {
      return n;
    }
    left -= n;
    offset += n;
  }
  return length;
}

} // namespace Sylar