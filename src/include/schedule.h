#ifndef __SYLAR_SCHEDULE_H__
#define __SYLAR_SCHEDULE_H__

#include "./coroutine.h"
#include "./log.h"
#include "./mutex.h"
#include "./thread"
#include "./util.h"
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace Sylar {

class Coroutine;

class Schedule {
public:
  friend Coroutine;

  typedef std::shared_ptr<Schedule> ptr;

  struct CoroutineTask {
    Coroutine::ptr coroutine_;
    std::function<void()> f_;
    int thread_id_;

    CoroutineTask() : coroutine_(nullptr), f_(nullptr), thread_id_(-1) {}
    CoroutineTask(const Coroutine::ptr p, int threadID = -1) {
      coroutine_ = p;
      thread_id_ = threadID;
    }
    CoroutineTask(const std::function<void()> &f, int threadID = -1) {
      f_ = f;
      thread_id_ = threadID;
    }
    void Reset() {
      coroutine_ = nullptr;
      f_ = nullptr;
      thread_id_ = -1;
    }
    void Reset(const std::function<void()> &f) {
      f_ = f;
      thread_id_ = -1, coroutine_ = nullptr;
    }
  };

  Schedule(size_t threads = 1, bool use_caller = true,
           const std::string &name = "");

  virtual ~Schedule();

  std::string GetName() const { return name_; }

  void SetName(const std::string &name) { name_ = name; }

  /**
   * @brief used to assign value to t_schedule
   *
   * @param ptr pass this pointer
   */
  void SetThis(const Schedule::ptr &ptr) { t_schedule = ptr; }

  static Schedule::ptr GetThis() { return t_schedule; }

  static Coroutine::ptr GetGlobalCo() { return t_global_coroutine; }

  void Start();

  void Stop();

  template <typename CoroutineOrFunc>
  bool ScheduleTask(CoroutineOrFunc cf, int thread = -1) {
    bool notify = false;
    {
      Mutex::ScopeLock l(mu_);
      notify = ScheduleNoLock(cf, thread);
    }
    if (notify) {
      Notify();
    }
    return notify;
  }

protected:
  /**
   * notify scheduler that a new task is added
   */
  virtual void Notify() {}

  /**
   * each coroutine running in single thread should execute this method. this
   * method will consistently execute task in tasks_. if all task run out, it
   * will execute Idel() method
   */
  void Run();

  /**
   * @return true, is stopped
   * @return false, is running
   */
  virtual bool IsStop();

  /**
   * when tasks is run out, coroutine can run this method
   */
  virtual void Idel();

private:
  template <typename CoroutineOrFunc>
  bool ScheduleNoLock(CoroutineOrFunc cf, int thread) {
    bool notify = tasks_.empty();
    CoroutineTask ct(cf, thread);
    if (ct.coroutine_ || ct.f_) {
      tasks_.push_back(ct);
    }
    return notify;
  }

private:
  std::string name_;
  Mutex mu_;
  std::vector<Thread::ptr> threads_; // therad pool
  std::list<CoroutineTask> tasks_;   // the task to be execute by coroutine

  /**
   * the following considersion is based on we at most only create one thread.
   *
   * concepts:
   *
   * schedule thread: a thread only used to schedule task(means that create
   * extra thread to execute these tasks).
   *
   * root_coroutine: only when we consider current thread as schedule thread and
   * execute thread, this concept exists. in current thread, we must extraly
   * create coroutine to perform schedule(whose meaning is iedntical with above)
   *
   * global_coroutine: our design of coroutine only support two coroutine swap,
   * if we want to support swap to root coroutine, we must use extra pointer
   * always point root coroutine
   *
   * if we not create new thread, means that we use current thread to schedule
   * and execute task; in contrast if we create a new thread, current thread can
   * used to schedule task and new thread can used to execute task.
   *
   * for the former one, we must create extra sub-coroutine to serve as schedule
   * coroutine
   *
   */
  Coroutine::ptr root_coroutine_;
  int root_thread_id_; // the thread id of root coroutine

  static thread_local Schedule::ptr t_schedule;
  static thread_local Coroutine::ptr t_global_coroutine;

protected:
  std::vector<int> thread_ids_;              // a vector used to track thread id
  size_t thread_num_;                        // the number of thread
  std::atomic<size_t> active_thread_num_{0}; // the number of active thread
  std::atomic<size_t> idel_thread_num_{0};   // the number of idel thread
  bool is_stoped_ = true;                    // stop or not
};

} // namespace Sylar

#endif