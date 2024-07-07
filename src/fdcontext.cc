#include "include/fdcontext.h"

namespace Sylar {

FDContext::FDContext(int fd)
    : fd_(fd), is_init_(false), is_socket_(false), is_close_(false),
      is_nonblock_(false) {
  Init();
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

FDManager::FDManager() { fds_.resize(64); }

FDContext::ptr FDManager::GetFD(int fd, bool auto_create) {
  if (fd < 0) {
    return nullptr;
  }
  Mutex::ScopeLock l(mu_);
  if (fd < fds_.size()) {
    return fds_[fd];
  }
  if (!auto_create) {
    return nullptr;
  }
  fds_.resize(fd * 2);
  fds_[fd] = std::make_shared<FDContext>(fd);
  return fds_[fd];
}

void FDManager::DeleteFD(int fd) {
  Mutex::ScopeLock l(mu_);
  if (fd >= fds_.size()) {
    return;
  }
  fds_[fd].reset();
}

} // namespace Sylar