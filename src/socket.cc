#include "include/socket.h"

namespace Sylar {

Socket::ptr Socket::CreateTCPSocket(const Address::ptr addr) {
  if (addr->GetFamily() == IPv4) {
    return CreateTCPIPv4Socket();
  } else if (addr->GetFamily() == IPv6) {
    return CreateTCPIPv6Socket();
  }
  return nullptr;
}

Socket::ptr Socket::CreateUDPSocket(const Address::ptr addr) {
  if (addr->GetFamily() == IPv4) {
    return CreateUDPIPv4Socket();
  } else if (addr->GetFamily() == IPv6) {
    return CreateUDPIPv6Socket();
  }
  return nullptr;
}

Socket::Socket(int family, int type, int protocol)
    : socket_(-1), family_(family), type_(type), protocol_(protocol),
      is_connected_(false) {

  if (type == UDP) {
    is_connected_ = true;
  }

  socket_ = socket(family_, type_, protocol_);

  FDManager::Instance().GetFD(socket_, true);

  if (socket_ == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::Socket socket create failed: " << strerror(errno);
    throw std::runtime_error(strerror(errno));
  }
}

Socket::~Socket() { Close(); }

bool Socket::GetSockOpt(int level, int option, void *result, socklen_t *len) {
  if (int ret = getsockopt(socket_, level, option, result, len); ret == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::GetSockOpt error: " << strerror(errno);
    return false;
  }
  return true;
}

bool Socket::SetSockOpt(int level, int option, void *result, socklen_t len) {
  if (int ret = setsockopt(socket_, level, option, result, len); ret == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::SetSockOpt error: " << strerror(errno);
    return false;
  }
  return true;
}

void Socket::Init(int socket) {
  socket_ = socket;
  is_connected_ = true;
  InitSocket();
}

void Socket::InitSocket() {
  int val = 1;
  SetSockOpt(SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  if (type_ == TCP) {
    SetSockOpt(IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
  }
}

bool Socket::Bind(const Address::ptr addr) {
  if (addr->GetFamily() != family_) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT) << "Socket::Bind error: family inconsist";
    return false;
  }
  if (::bind(socket_, addr->GetSockaddr(), addr->GetSocklen()) != 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::Bind error: " << strerror(errno);
    return false;
  }
  return true;
}

bool Socket::Listen(int backlog) {
  if (::listen(socket_, backlog) != 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::Listen error: " << strerror(errno);
    return false;
  }
  return true;
}

Socket::ptr Socket::Accept() {
  int fd = ::accept(socket_, nullptr, nullptr);
  if (fd == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::Accept error: " << strerror(errno);
    return nullptr;
  }
  Socket::ptr ret(new Socket(family_, type_, protocol_));
  ret->Init(fd);
  return ret;
}

bool Socket::Connect(const Address::ptr addr) {
  if (::connect(socket_, addr->GetSockaddr(), addr->GetSocklen()) != 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::Connect error: " << strerror(errno);
    return false;
  }
  is_connected_ = true;
  return true;
}

bool Socket::Close() {
  is_connected_ = false;
  if (socket_ != -1) {
    ::close(socket_);
    socket_ = -1;
  }
  return true;
}

uint64_t Socket::GetSendTimeout() {
  auto fd = FDManager::Instance().GetFD(socket_);
  if (fd) {
    return fd->GetTimeout(SO_SNDTIMEO);
  }
  return 0;
}

uint64_t Socket::GetRecvTimeout() {
  auto fd = FDManager::Instance().GetFD(socket_);
  if (fd) {
    return fd->GetTimeout(SO_RCVTIMEO);
  }
  return 0;
}

void Socket::SetRecvTimeout(uint64_t v) {
  struct timeval tv {
    int64_t(v) / 1000, int64_t(v) / 1000000
  };
  SetSockOpt(SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  auto fd = FDManager::Instance().GetFD(socket_);
  if (fd) {
    fd->SetTimeout(SO_RCVTIMEO, v);
  }
}

void Socket::SetSendTimeout(uint64_t v) {
  struct timeval tv {
    int64_t(v) / 1000, int64_t(v) / 1000000
  };
  SetSockOpt(SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  auto fd = FDManager::Instance().GetFD(socket_);
  if (fd) {
    fd->SetTimeout(SO_SNDTIMEO, v);
  }
}

ssize_t Socket::Send(const void *buf, size_t len, int flags) {
  int ret = ::send(socket_, buf, len, flags);
  if (ret == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::Send error: " << strerror(errno);
  }
  return ret;
}

ssize_t Socket::SendTo(const void *buf, size_t len, Address::ptr to,
                       int flags) {
  int ret =
      ::sendto(socket_, buf, len, flags, to->GetSockaddr(), to->GetSocklen());
  if (ret == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::SendTo error: " << strerror(errno);
  }
  return ret;
}

ssize_t Socket::SendTo(const iovec *iov, size_t length, Address::ptr to,
                       int flags) {
  msghdr msg;
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = (iovec *)iov;
  msg.msg_iovlen = length;
  msg.msg_name = to->GetSockaddr();
  msg.msg_namelen = to->GetSocklen();
  return ::sendmsg(socket_, &msg, flags);
}

ssize_t Socket::SendMsg(const msghdr *msg, int flags) {
  int ret = ::sendmsg(socket_, msg, flags);
  if (ret == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::SendMsg error: " << strerror(errno);
  }
  return ret;
}

ssize_t Socket::Recv(void *buf, size_t len, int flags) {
  int ret = ::recv(socket_, buf, len, flags);
  if (ret == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::Recv error: " << strerror(errno);
  }
  return ret;
}

ssize_t Socket::RecvFrom(void *buf, size_t len, Address::ptr from, int flags) {
  socklen_t slen = from->GetSocklen();
  int ret = ::recvfrom(socket_, buf, len, flags, from->GetSockaddr(), &slen);
  if (ret == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::RecvFrom error: " << strerror(errno);
  }
  return ret;
}

ssize_t Socket::RecvMsg(msghdr *msg, int flags) {
  int ret = ::recvmsg(socket_, msg, flags);
  if (ret == -1) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Socket::RecvMsg error: " << strerror(errno);
  }
  return ret;
}

ssize_t Socket::RecvFrom(iovec *iov, size_t length, Address::ptr from,
                         int flags) {
  msghdr msg;
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = iov;
  msg.msg_iovlen = length;
  msg.msg_name = from->GetSockaddr();
  msg.msg_namelen = from->GetSocklen();
  return ::recvmsg(socket_, &msg, flags);
}

std::string Socket::ToString() {
  std::stringstream ss;
  ss << "[socket = " << socket_ << ", connect = " << is_connected_
     << ", family = " << family_ << ", type = " << type_
     << ", protocol = " << protocol_ << "]" << std::endl;
  return ss.str();
}
} // namespace Sylar