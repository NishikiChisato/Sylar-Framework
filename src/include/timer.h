#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include "./mutex.h"
#include "./util.h"
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Sylar {

class TimeManager;

class Timer : public std::enable_shared_from_this<Timer> {
  friend TimeManager;

public:
  typedef std::shared_ptr<Timer> ptr;

  /**
   * @brief reset the interval of this timer
   *
   * @param [in] interval the new interval
   * @param [in] from_now if this parameter is set to true, the next trigger
   * time will calculate from now; otherwise, the next trigger time will
   * calculate from last trigger time
   */
  void Reset(uint64_t interval, bool from_now);

  /**
   * @brief refresh the execute time
   */
  void Refresh();

  /**
   * @brief cancel this timer
   *
   */
  void Cancel();

private:
  /**
   * @brief Construct a new Timer object
   *
   * @param name        the name of this timer
   * @param interval    the interval of this timer(ms)
   * @param recurrence  recurrence or not
   * @param func        callback function
   */
  Timer(const std::string &name, uint64_t interval, std::function<void()> func,
        TimeManager *tmgr, bool recurrence = false);

  Timer(uint64_t now);

private:
  std::string name_;           // the name of this timer
  bool is_recurrence_ = false; // recurrce execute
  uint64_t interval_;          // interval of execute(ms)
  uint64_t next_trigger_;      // time of next trigger(ms)
  std::function<void()> func_; // callback function
  /**
   * user cannot directly create Timer object, user must use TimeManager to
   * indirectly create Time object
   *
   * since we use std::set to store Timer object, when we want to update a timer
   * object, we must erase it from std::set and insert it into std::set(std::set
   * is ordered container)
   */
  TimeManager *tmgr_;

  struct Comparator {
    bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
  };
};

class TimeManager {
  friend Timer;

public:
  typedef std::shared_ptr<TimeManager> ptr;

  TimeManager();

  virtual ~TimeManager() {}

  /**
   * @brief used to add a timer
   *
   * @param name          the name of timer
   * @param interval      the interval of timer
   * @param func          the callback function of timer
   * @param recurrence    recurrence or not
   * @return Timer::ptr   the pointer of created timer
   */
  Timer::ptr AddTimer(const std::string &name, uint64_t interval,
                      std::function<void()> func, bool recurrence = false);

  /**
   * @brief used to add a timer with trigger condition
   *
   * @attention we use weak_ptr to check whether the condition is satified. if
   * weak_ptr is not nullptr, we think of the condition is satified
   *
   * @param name          the name of timer
   * @param interval      the interval of timer
   * @param func          the callback function of timer
   * @param condition     trigger condition
   * @param recurrence    recurrence or not
   * @return Timer::ptr   the pointer of created timer
   */
  Timer::ptr AddTimerWithCondition(const std::string &name, uint64_t interval,
                                   std::function<void()> func,
                                   std::weak_ptr<void> condition,
                                   bool recurrence = false);

  void GetAllExpired(uint64_t now, std::vector<std::function<void()>> &vfunc);

  uint64_t GetNextTriggerTime(uint64_t now);

  bool IsStop() { return timers_.empty(); }

private:
  /**
   * @brief used to detece whether server time is set back
   *
   * @return true is set back; otherwise is not
   */
  bool DetectTimerSetBack(uint64_t now);

private:
  std::set<Timer::ptr, Timer::Comparator> timers_;
  Mutex mu_;
  uint64_t previous_trigger_;
};

} // namespace Sylar

#endif