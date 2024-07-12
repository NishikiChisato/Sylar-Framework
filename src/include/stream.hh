#ifndef __SYLAR_STREAM_H__
#define __SYLAR_STREAM_H__

#include "bytearray.hh"

namespace Sylar {

class Stream {
public:
  typedef std::shared_ptr<Stream> ptr;
  ~Stream() {}

  virtual size_t Read(void *buffer, size_t length) = 0;

  virtual size_t Read(ByteArray::ptr ba, size_t length) = 0;

  /**
   * @brief guarantee read fixed size data
   */
  virtual size_t ReadFixedSize(void *buffer, size_t length);

  virtual size_t ReadFixedSize(ByteArray::ptr ba, size_t length);

  virtual size_t Write(const void *buffer, size_t length) = 0;

  virtual size_t Write(ByteArray::ptr ba, size_t length) = 0;

  /**
   * @brief guarantee write fixed size data
   */
  virtual size_t WriteFixedSize(const void *buffer, size_t length);

  virtual size_t WriteFixedSize(ByteArray::ptr ba, size_t length);

  virtual void Close() = 0;

private:
};

} // namespace Sylar

#endif