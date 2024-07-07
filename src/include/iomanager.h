#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "./coroutine.h"
#include "./mutex.h"
#include "./schedule.h"
#include "./timer.h"
#include <algorithm>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace Sylar {

class Timer;
class TimeManager;

class IOManager : public Schedule, public TimeManager {
public:
  typedef std::shared_ptr<IOManager> ptr;

  /**
   * we only define two events: read event and write event. the rest of events
   * in epoll will be grouped into these two event.
   */
  enum Event {
    NONE = 0x0, // non event
    READ = 0x1, // read event(EPOLLIN)
    WRITE = 0x4 // write event(EPOLLOUT)
  };

private:
  /**
   * for IO scheduling, each schedule task contain a tuple: file descriptor,
   * event type and callback function
   *
   * the former two used by epoll_wait and the latter used by scheduler
   */
  struct FDContext {
    typedef std::shared_ptr<FDContext> ptr;
    struct EventContext {
      Schedule *scheduler_ =
          nullptr;                 // only used to execute coroutine or function
      Coroutine::ptr coroutine_;   // the coroutine to be executed of this event
      std::function<void()> func_; // the callback function of this event
    };

    /**
     * @brief Get the context of event
     *
     * @param[in] event event type
     * @return return the context of event
     */
    EventContext &GetEventContext(Event event);

    /**
     * @brief reset context of event
     *
     * @attention this function merely reset one context at a time
     *
     * @param[in] event the type of event will be reset. the type of event
     * merely can be read and write
     */
    void ResetEventContext(Event event);

    /**
     * @brief trigger event
     * @details use scheduler to schedule and execute coroutine or function
     *
     * @param[in] event event type
     */
    void TriggerEvent(Event event);

    std::string DumpToString() {
      std::stringstream ss;
      ss << "event: " << EventToString(event_)
         << ", fd: " << std::to_string(fd_) << std::endl;

      if (read_ctx_.scheduler_) {
        ss << "[read schedule]" << std::endl;
      }
      if (read_ctx_.func_) {
        ss << "[read func]" << std::endl;
      }
      if (read_ctx_.coroutine_) {
        ss << "[read coroutine]" << std::endl;
      }

      if (write_ctx_.scheduler_) {
        ss << "[write schedule]" << std::endl;
      }
      if (write_ctx_.func_) {
        ss << "[write func]" << std::endl;
      }
      if (write_ctx_.coroutine_) {
        ss << "[write coroutine]" << std::endl;
      }
      return ss.str();
    }

    EventContext read_ctx_;  // the context of read event
    EventContext write_ctx_; // the context of write event
    int fd_;                 // file descriptor
    Event event_ = NONE;     // the event type intersted by file descriptor
    Mutex mu_;
  };

  void ExtendFDContext(size_t sz);

public:
  IOManager(size_t threads = 1, bool use_caller = true,
            const std::string &name = "");
  ~IOManager();

  static std::string EventToString(Event event);

  /**
   * @brief add a monitoring event to fd, when event occur, func will be
   * executed
   *
   * @param [in] fd file descriptor
   * @param [in] event event type
   * @param [in] func callback function
   * @return when successful, return 0; else return 1
   */
  bool AddEvent(int fd, Event event, std::function<void()> func = nullptr);

  bool AddEvent(int fd, Event event, Coroutine::ptr coroutine);

  /**
   * @brief delete some monitoring event from fd. if monitoring event set is
   * empty, we will cal epoll_ctl to delete all event of this fd
   *
   * @attention this function will not trigger callback function
   *
   * @param [in] fd file descriptor
   * @param [in] event event type
   * @return when successful, return 0; else return 1
   */
  bool DelEvent(int fd, Event event);

  /**
   * @brief the behavior of this methor is identical to DelEvent, excpet that it
   * will trigger callback function
   *
   * @attention if this event to be deleted has been registered callback
   * function, we will execute that callback function
   *
   * @param [in] fd file descriptor
   * @param [in] event event type
   * @return when successful, return 0; else return 1
   */
  bool CancelEvent(int fd, Event event);

  /**
   * @brief the behavior of this method is identical to CancelEvent, excpet that
   * it will cancel all event from fd
   *
   * @param fd file descriptor
   * @return when successful, return 0; else return 1
   */
  bool CancelAllEvent(int fd);

  std::string ListAllEvent();

  static IOManager *GetThis() {
    return dynamic_cast<IOManager *>(Schedule::GetThis());
  }

protected:
  void Notify() override;
  void Idel() override;
  bool IsStop() override {
    return pending_event_ == 0 && TimeManager::IsStop() && Schedule::IsStop();
  }

private:
  int epfd_ = 0;       // the file descriptor of epoll
  int notify_pipe_[2]; // the pipe of file descriptor to tickle, read end is
                       // fd[0], write end is fd[1]
  std::atomic<size_t> pending_event_{0}; // the number of pending event
  Mutex mu_;                             // exclusive lock
  /**
   * we cannot use smart_ptr in here
   *
   * when we want to expend the size of vector, we would not perform deep copy
   *
   * we delete all elements in destructor
   */
  std::vector<FDContext *>
      fd_context_; // the vector of context of file descriptor
};

} // namespace Sylar

#endif