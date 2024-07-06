#include "include/timer.h"

namespace Sylar {

bool Timer::Comparator::operator()(const Timer::ptr &lhs,
                                   const Timer::ptr &rhs) const {
  if (!lhs) {
    return true;
  } else if (!rhs) {
    return false;
  } else {
    return lhs->next_trigger_ < rhs->next_trigger_;
  }
}

Timer::Timer(const std::string &name, uint64_t interval,
             std::function<void()> func, TimeManager *tmgr, bool recurrence)
    : name_(name), interval_(interval), is_recurrence_(recurrence), func_(func),
      tmgr_(tmgr) {
  next_trigger_ = GetElapseFromRebootMS() + interval_;
}

Timer::Timer(uint64_t now) : next_trigger_(now) {}

void Timer::Refresh() {
  Mutex::ScopeLock l(tmgr_->mu_);
  auto it = tmgr_->timers_.find(shared_from_this());
  if (it == tmgr_->timers_.end()) {
    return;
  }
  tmgr_->timers_.erase(it);
  int64_t now = GetElapseFromRebootMS();
  next_trigger_ = now + interval_;
  tmgr_->timers_.insert(shared_from_this());
}

void Timer::Reset(uint64_t interval, bool from_now) {
  Mutex::ScopeLock l(tmgr_->mu_);
  tmgr_->timers_.erase(shared_from_this());
  int64_t now = GetElapseFromRebootMS();
  int64_t start = 0;
  if (from_now) {
    start = now;
  } else {
    start = next_trigger_ - interval_;
  }
  interval_ = interval;
  next_trigger_ = start + interval_;
  tmgr_->timers_.insert(shared_from_this());
}

void Timer::Cancel() {
  Mutex::ScopeLock l(tmgr_->mu_);
  auto it = tmgr_->timers_.find(shared_from_this());
  tmgr_->timers_.erase(it);
  func_ = nullptr;
}

TimeManager::TimeManager() { previous_trigger_ = GetElapseFromRebootMS(); }

bool TimeManager::DetectTimerSetBack(uint64_t now) {
  bool set_back = false;
  /**
   * the resoulution of now is millisecond, we cannot directly use
   * now < previous_trigger_ to judge current server is set back, we should use
   * a range to judge
   *
   * we must convert to int64_t to compare, uint64_t may overflow
   */
  if ((int64_t)now < (int64_t)previous_trigger_ - (60 * 60 * 1000)) {
    set_back = true;
    previous_trigger_ = now;
  }
  return set_back;
}

static void ConditionTimer(std::weak_ptr<void> cond,
                           std::function<void()> func) {
  std::shared_ptr<void> ptr = cond.lock();
  /**
   * weak_ptr will not increase the reference count, we can use weak_ptr.lock()
   * to check whether pointed object is alive(if return nullptr, means that it
   * has destory).
   */
  if (ptr) {
    func();
  }
}

Timer::ptr TimeManager::AddTimer(const std::string &name, uint64_t interval,
                                 std::function<void()> func, bool recurrence) {
  Timer::ptr new_timer(new Timer(name, interval, func, this, recurrence));

  Mutex::ScopeLock l(mu_);
  timers_.insert(new_timer);
  return new_timer;
}

Timer::ptr TimeManager::AddTimerWithCondition(const std::string &name,
                                              uint64_t interval,
                                              std::function<void()> func,
                                              std::weak_ptr<void> condition,
                                              bool recurrence) {
  return AddTimer(name, interval, std::bind(&ConditionTimer, condition, func),
                  recurrence);
}

void TimeManager::GetAllExpired(uint64_t now,
                                std::vector<std::function<void()>> &vfunc) {
  Mutex::ScopeLock l(mu_);
  if (timers_.empty()) {
    return;
  }
  bool is_set_back = DetectTimerSetBack(now);

  if (!is_set_back && (*timers_.begin())->next_trigger_ > now) {
    return;
  }

  Timer::ptr now_timer(new Timer(now));
  // find the first timer, which is greater than and queal to now_timer in set
  auto it = is_set_back ? timers_.end() : timers_.lower_bound(now_timer);

  // insert timers into expired_timers and delete timers from set
  std::vector<Timer::ptr> expired_timers;
  expired_timers.insert(expired_timers.begin(), timers_.begin(), it);
  timers_.erase(timers_.begin(), it);

  for (auto &it : expired_timers) {
    vfunc.push_back(it->func_);
    if (it->is_recurrence_) {
      it->next_trigger_ = now + it->interval_;
      timers_.insert(it);
    }
  }
}

bool TimeManager::HasExpired() {
  Mutex::ScopeLock l(mu_);
  if (timers_.empty()) {
    return false;
  }
  uint64_t now = GetElapseFromRebootMS();
  auto it = timers_.begin();
  if (now >= (*it)->next_trigger_) {
    return true;
  }
  return false;
}

uint64_t TimeManager::GetNextTriggerTime(uint64_t now) {
  Mutex::ScopeLock l(mu_);
  if (timers_.empty()) {
    return ~0ull;
  }
  auto it = timers_.begin();
  if ((*it)->next_trigger_ <= now) {
    return 0;
  } else {
    return (*it)->next_trigger_ - now;
  }
}

} // namespace Sylar