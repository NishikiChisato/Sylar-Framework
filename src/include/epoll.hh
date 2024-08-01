#ifndef __SYLAR_EPOLL_HH__
#define __SYLAR_EPOLL_HH__

#include "hook.hh"
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
  typedef std::shared_ptr<Epoll> ptr;
  friend Schedule;

  enum EventType { NONE = 0x0, READ = 0x1, WRITE = 0x4 };

  struct EventCtx {
    EventCtx()
        : type_(0), fd_(-1), r_callback_(nullptr), r_co_(nullptr),
          w_callback_(nullptr), w_co_(nullptr), ptr_(nullptr) {}
    int type_;
    int fd_;
    // when listened event trigger, this funciton should be executed. attention:
    // coroutine won't execute this function
    std::function<void()> r_callback_;
    std::function<void()> w_callback_;
    std::shared_ptr<Coroutine> r_co_;
    std::shared_ptr<Coroutine> w_co_;
    void *ptr_; // user can pass a pointer to EventCtx
  };

  ~Epoll();

  static std::shared_ptr<Epoll> Instance() {
    static thread_local std::shared_ptr<Epoll> instance(new Epoll());
    return instance;
  }

  static std::shared_ptr<Epoll> GetThreadEpoll();

  static void InitThreadEpoll();

  void StopEventLoop(bool val) { loop_ = val; }

  bool Stoped() { return loop_; }

  /**
   * @brief execute callback function when listened event occurs or resuming
   * coroutine when listened event occurs
   *
   * @attention if coroutine is nullptr, we will run callback function in
   * current thread
   *
   * @attention if *_func and *_co both have value, we will execute function and
   * ignore coroutine
   *
   * @param type    type to be listened
   * @param fd      file descriptor
   * @param r_func  when fd can be read, execute this functione
   * @param w_func  when fd can be written, execute this function
   * @param r_co    when fd can be read, resume this coroutine
   * @param w_co    when fd can be written, resume this coroutine
   */
  void RegisterEvent(int type, int fd, std::function<void()> r_func,
                     std::function<void()> w_func,
                     std::shared_ptr<Coroutine> r_co = nullptr,
                     std::shared_ptr<Coroutine> w_co = nullptr);

  void RegisterEvent(int type, int fd, std::shared_ptr<EventCtx> ptr);

  void CancelEvent(int type, int fd);

  Epoll() : TimeWheel(), epfd_(false), loop_(false) {}

  int epfd_;
  std::unordered_map<int, std::shared_ptr<EventCtx>> reg_event_;
  bool loop_;

  static thread_local std::shared_ptr<Epoll> t_epoll_;
  static thread_local int64_t reference_cnt_;
};

} // namespace Sylar

#endif