#ifndef __SYLAR_EPOLL_HH__
#define __SYLAR_EPOLL_HH__

#include "timewheel.hh"
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unordered_map>

namespace Sylar {

class Schedule;

class Epoll : public TimeWheel {
public:
  friend Schedule;

  enum EventType { NONE = 0x0, READ = 0x1, WRITE = 0x2 };

  struct EventCtx {
    int type_ = EventType::NONE;
    // when listened event trigger, this funciton should be executed. attention:
    // coroutine won't execute this function
    std::function<void()> callback_;
  };

  ~Epoll() = default;

  static std::shared_ptr<Epoll> Instance() {
    static thread_local std::shared_ptr<Epoll> instance(new Epoll());
    return instance;
  }

  static void InitEpoll(std::shared_ptr<Epoll> ep);

  static void StopEventLoop(bool val) { loop_ = val; }

  static bool Stoped() { return loop_; }

  static void RegisterEvent(EventType type, int fd, std::function<void()> func);

  static void CancelEvent(EventType type, int fd);

private:
  Epoll() : TimeWheel() {}

  static int epfd_;
  static std::unordered_map<int, EventCtx> reg_event_;
  static bool loop_;
};

} // namespace Sylar

#endif