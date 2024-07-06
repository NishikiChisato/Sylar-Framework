#include "include/fdcontext.h"

namespace Sylar {

FDContext::FDContext(int fd)
    : fd_(fd), is_init_(false), is_socket_(false), is_close_(false),
      is_sys_nonblock_(false), is_user_nonblock_(false), read_timeout_(~0ull),
      write_timeout_(~0ull) {
  Init();
}

uint64_t FDContext::GetTimeout(int type) {
  if (type == SO_RCVTIMEO) {
    return read_timeout_;
  } else if (type == SO_SNDTIMEO) {
    return write_timeout_;
  }
  return ~0ull;
}

void FDContext::SetTimeout(int type, uint64_t timeout) {
  if (type == SO_RCVTIMEO) {
    read_timeout_ = timeout;
  } else if (type == SO_SNDTIMEO) {
    write_timeout_ = timeout;
  }
  return;
}

void FDContext::SetNonBlock(bool v) {
  is_nonblock_ = v;
  int flag = fcntl(fd_, F_GETFL);
  fcntl(fd_, flag & ~O_NONBLOCK);
}

void FDContext::Init() {
  if (is_init_) {
    return;
  }
  struct stat state;
  if (-1 == fstat(fd_, &state)) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "fstat error on: " << fd_ << ", err is " << strerror(errno);
  } else {
    is_init_ = true;
    is_socket_ = S_ISSOCK(state.st_mode);
  }
  if (is_socket_) {
    int flag = fcntl(fd_, F_GETFL);
    fcntl(fd_, F_SETFL, flag | O_NONBLOCK);
    is_nonblock_ = true;
  }
}

} // namespace Sylar