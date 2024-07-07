#include "include/fdcontext.h"

namespace Sylar {

FDContext::FDContext(int fd)
    : fd_(fd), is_init_(false), is_socket_(false), is_fifo_(false),
      is_close_(false), is_nonblock_(false) {
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
    if (errno == EBADF) {
      is_close_ = true;
    }
  }

  is_init_ = true;
  if (S_ISSOCK(state.st_mode)) {
    is_socket_ = true;
  }

  if (S_ISFIFO(state.st_mode)) {
    is_fifo_ = true;
  }

  int flag = fcntl(fd_, F_GETFL);
  if (flag & O_NONBLOCK) {
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