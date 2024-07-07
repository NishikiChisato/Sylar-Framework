#include "include/hook.h"

static bool is_hook_enable = true;

namespace Sylar {
bool GetHookEnable() { return is_hook_enable; }
void SetHookEnable(bool val) { is_hook_enable = val; }

#define HOOK_FUNC(XX)                                                          \
  XX(sleep)                                                                    \
  XX(usleep)                                                                   \
  XX(read)                                                                     \
  XX(readv)                                                                    \
  XX(recv)                                                                     \
  XX(recvfrom)                                                                 \
  XX(recvmsg)                                                                  \
  XX(write)                                                                    \
  XX(writev)                                                                   \
  XX(send)                                                                     \
  XX(sendto)                                                                   \
  XX(sendmsg)                                                                  \
  XX(close)

void hook_init() {
  static bool is_init = false;
  if (is_init) {
    return;
  }
}

struct _HookIniter {
  _HookIniter() { hook_init(); }
};

// a variable declared as static will initialize before execcution of main
// function
static _HookIniter _hook_initer;

template <typename OriginFunc, typename... Args>
static ssize_t do_io(int fd, OriginFunc func, uint32_t event, Args &&...args) {
  if (!Sylar::GetHookEnable()) {
    return func(fd, std::forward<Args>(args)...);
  }
  auto fdptr = Sylar::FDManager::Instance().GetFD(fd);
  if (!fdptr) {
    return func(fd, std::forward<Args>(args)...);
  }
  if (fdptr->IsClose()) {
    errno = EBADF;
    return -1;
  }
  if (!fdptr->IsSocket() || !fdptr->IsNonBlock()) {
    return func(fd, std::forward<Args>(args)...);
  }

retry:
  ssize_t n = func(fd, std::forward<Args>(args)...);
  if (n == -1 && errno == EINTR) {
    // if current call is interupted by SIGNAL, we should retry
    n = func(fd, std::forward<Args>(args)...);
  }
  if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    // if current call return immediately
    auto iomgr = Sylar::IOManager::GetThis();
    iomgr->AddEvent(fd, (Sylar::IOManager::Event)event);
    iomgr->ScheduleTask(Sylar::Coroutine::GetThis());
    Sylar::Coroutine::YieldToReady();
    goto retry;
  }
  return n;
}

} // namespace Sylar

extern "C" {

// initialize to nullptr
// #define XX(name) name##_func name##_f = nullptr;
// HOOK_FUNC(XX);
// #undef XX

#define XX(name) name##_func name##_f = (name##_func)dlsym(RTLD_NEXT, #name);
HOOK_FUNC(XX)
#undef XX

unsigned int sleep(unsigned int seconds) {
  if (!Sylar::GetHookEnable()) {
    return sleep_f(seconds);
  }
  auto iomgr = Sylar::IOManager::GetThis();
  iomgr->AddTimer("", seconds * 1000, nullptr);
  iomgr->ScheduleTask(Sylar::Coroutine::GetThis());
  Sylar::Coroutine::YieldToReady();
  return 0;
}

int usleep(useconds_t usec) {
  if (!Sylar::GetHookEnable()) {
    return usleep_f(usec);
  }
  auto iomgr = Sylar::IOManager::GetThis();
  iomgr->AddTimer("", usec / 1000, nullptr);
  iomgr->ScheduleTask(Sylar::Coroutine::GetThis());
  Sylar::Coroutine::YieldToReady();
  return 0;
}

ssize_t read(int fd, void *buf, size_t count) {
  return Sylar::do_io(fd, read_f, Sylar::IOManager::READ, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  return Sylar::do_io(fd, readv_f, Sylar::IOManager::READ, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  return Sylar::do_io(sockfd, recv_f, Sylar::IOManager::READ, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
  return Sylar::do_io(sockfd, recvfrom_f, Sylar::IOManager::READ, buf, len,
                      flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return Sylar::do_io(sockfd, recvmsg_f, Sylar::IOManager::READ, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
  return Sylar::do_io(fd, write_f, Sylar::IOManager::WRITE, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  return Sylar::do_io(fd, write_f, Sylar::IOManager::WRITE, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  return Sylar::do_io(sockfd, send_f, Sylar::IOManager::WRITE, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
  return Sylar::do_io(sockfd, sendto_f, Sylar::IOManager::WRITE, buf, len,
                      flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  return Sylar::do_io(sockfd, sendmsg_f, Sylar::IOManager::WRITE, msg, flags);
}
}
