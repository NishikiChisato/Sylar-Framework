#include "include/hook.hh"
#include "include/coroutine.hh"
#include "include/epoll.hh"
#include "include/fdcontext.hh"

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
  // fdcontext is used to set nonblock
  auto fdptr = FDManager::Instance().GetFD(fd, true);

  if (!fdptr) {
    return func(fd, std::forward<Args>(args)...);
  }
  if (fdptr->IsClose()) {
    errno = EBADF;
    return -1;
  }
  if ((!fdptr->IsSocket() && !fdptr->IsFifo()) || !fdptr->IsNonBlock()) {
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
    auto epoll = Sylar::Schedule::GetThreadEpoll();
    if (event & Sylar::Epoll::EventType::READ) {
      epoll->RegisterEvent((Sylar::Epoll::EventType)event, fd, nullptr, nullptr,
                           Sylar::Schedule::GetCurrentCo());
    }
    if (event & Sylar::Epoll::EventType::WRITE) {
      epoll->RegisterEvent((Sylar::Epoll::EventType)event, fd, nullptr, nullptr,
                           nullptr, Sylar::Schedule::GetCurrentCo());
    }
    // main coroutine may invoke these function, we cannot yield main coroutine
    if (Sylar::Schedule::GetInvokeDeepth() >= 2) {
      Sylar::Schedule::Yield();
    }
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
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "hook sleep execute";

  auto epoll = Sylar::Schedule::GetThreadEpoll();
  epoll->AddTimer(seconds * 1000, nullptr, Sylar::Schedule::GetCurrentCo());
  Sylar::Schedule::Yield();

  return 0;
}

int usleep(useconds_t usec) {
  if (!Sylar::GetHookEnable()) {
    return usleep_f(usec);
  }
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "hook usleep execute";
  auto epoll = Sylar::Schedule::GetThreadEpoll();
  epoll->AddTimer(usec / 1000, nullptr, Sylar::Schedule::GetCurrentCo());
  Sylar::Schedule::Yield();
  return 0;
}

ssize_t read(int fd, void *buf, size_t count) {
  return Sylar::do_io(fd, read_f, Sylar::Epoll::EventType::READ, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  return Sylar::do_io(fd, readv_f, Sylar::Epoll::EventType::READ, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  return Sylar::do_io(sockfd, recv_f, Sylar::Epoll::EventType::READ, buf, len,
                      flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
  return Sylar::do_io(sockfd, recvfrom_f, Sylar::Epoll::EventType::READ, buf,
                      len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return Sylar::do_io(sockfd, recvmsg_f, Sylar::Epoll::EventType::READ, msg,
                      flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
  return Sylar::do_io(fd, write_f, Sylar::Epoll::EventType::WRITE, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  return Sylar::do_io(fd, write_f, Sylar::Epoll::EventType::WRITE, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  return Sylar::do_io(sockfd, send_f, Sylar::Epoll::EventType::WRITE, buf, len,
                      flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
  return Sylar::do_io(sockfd, sendto_f, Sylar::Epoll::EventType::WRITE, buf,
                      len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  return Sylar::do_io(sockfd, sendmsg_f, Sylar::Epoll::EventType::WRITE, msg,
                      flags);
}

int close(int fd) {
  if (!Sylar::GetHookEnable()) {
    return close_f(fd);
  }
  auto fdptr = Sylar::FDManager::Instance().GetFD(fd);
  if (fdptr) {
    auto epoll = Sylar::Schedule::GetThreadEpoll();
    epoll->CancelEvent(Sylar::Epoll::EventType::READ, fd);
    epoll->CancelEvent(Sylar::Epoll::EventType::WRITE, fd);
    Sylar::FDManager::Instance().DeleteFD(fd);
  }
  return close_f(fd);
}
}
