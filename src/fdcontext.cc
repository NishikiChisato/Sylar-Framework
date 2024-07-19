#include "include/fdcontext.hh"

namespace Sylar {

FDContext::FDContext(int fd)
    : fd_(fd), is_socket_(false), is_fifo_(false), is_close_(false),
      is_nonblock_(false), recv_timeout_(-1), send_timeout_(-1) {
  struct stat state;
  if (-1 == fstat(fd_, &state)) {
    if (errno == EBADF) {
      is_close_ = true;
    }
  }

  if (S_ISSOCK(state.st_mode)) {
    is_socket_ = true;
  }

  if (S_ISFIFO(state.st_mode)) {
    is_fifo_ = true;
  }

  int flag = fcntl(fd_, F_GETFL);
  fcntl(fd_, F_SETFL, flag | O_NONBLOCK);
  is_nonblock_ = true;
}

void FDContext::SetNonBlock(bool v) {
  is_nonblock_ = v;
  int flag = fcntl(fd_, F_GETFL);
  if (is_nonblock_) {
    fcntl(fd_, flag | O_NONBLOCK);
  } else {
    fcntl(fd_, flag & ~O_NONBLOCK);
  }
}

void FDContext::SetTimeout(int type, uint64_t v) {
  if (type == SO_RCVTIMEO) {
    recv_timeout_ = v;
  } else if (type == SO_SNDTIMEO) {
    send_timeout_ = v;
  }
}

uint64_t FDContext::GetTimeout(int type) {
  if (type == SO_RCVTIMEO) {
    return recv_timeout_;
  } else if (type == SO_SNDTIMEO) {
    return send_timeout_;
  }
  return 0;
}

FDContext::ptr FDManager::GetFD(int fd, bool auto_create) {
  if (fd < 0) {
    return nullptr;
  }
  Mutex::ScopeLock l(mu_);
  auto it = fds_.find(fd);
  if (it != fds_.end()) {
    return it->second;
  } else if (!auto_create) {
    return nullptr;
  }
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