#ifndef __SYLAR_SOCKETSTREAM_H__
#define __SYLAR_SOCKETSTREAM_H__

#include "socket.hh"
#include "stream.hh"

namespace Sylar {

class SocketStream : public Stream {
public:
  typedef std::shared_ptr<SocketStream> ptr;

  SocketStream(Socket::ptr ptr) : socket_(ptr) {}

  virtual ~SocketStream();

  /**
   * @attention this two read function cannot guarantee read length bits data
   */
  virtual size_t Read(void *buffer, size_t length) override;

  virtual size_t Read(ByteArray::ptr ba, size_t length) override;

  /**
   * @attention this two write function cannot guarantee write length bits data
   */
  virtual size_t Write(const void *buffer, size_t length) override;

  virtual size_t Write(ByteArray::ptr ba, size_t length) override;

  virtual void Close() override;

  bool IsConnected();

private:
  Socket::ptr socket_;
};

} // namespace Sylar

#endif