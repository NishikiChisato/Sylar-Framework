#include "include/socketstream.hh"

namespace Sylar {

SocketStream::~SocketStream() {
  if (socket_) {
    socket_->Close();
  }
}

size_t SocketStream::Read(void *buffer, size_t length) {
  if (IsConnected()) {
    return socket_->Recv(buffer, length);
  }
  return 0;
}

size_t SocketStream::Read(ByteArray::ptr ba, size_t length) {
  if (IsConnected()) {
    std::vector<iovec> ivec;
    ba->GetBuffer(ivec, length);
    size_t sz = 0;
    for (auto &iov : ivec) {
      sz += socket_->Recv(iov.iov_base, iov.iov_len);
    }
    return sz;
  }
  return 0;
}

size_t SocketStream::Write(const void *buffer, size_t length) {
  if (IsConnected()) {
    return socket_->Send(buffer, length);
  }
  return 0;
}

size_t SocketStream::Write(ByteArray::ptr ba, size_t length) {
  if (IsConnected()) {
    std::vector<iovec> ivec;
    ba->GetBuffer(ivec, length);
    size_t sz = 0;
    for (auto &iov : ivec) {
      sz += socket_->Send(iov.iov_base, iov.iov_len);
    }
    return sz;
  }
  return 0;
}

void SocketStream::Close() {}

bool SocketStream::IsConnected() {
  return socket_ ? socket_->IsConnected() : false;
}

} // namespace Sylar