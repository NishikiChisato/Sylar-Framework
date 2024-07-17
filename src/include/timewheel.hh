#ifndef __SYLAR_TIMEWHEEL_HH__
#define __SYLAR_TIMEWHEEL_HH__

#include "util.hh"
#include <functional>
#include <list>
#include <memory>
#include <thread>
#include <vector>

namespace Sylar {

class Coroutine;

// this class represent a timeout event. we use list to organize each timeout
// event in each node of list, we have a field point to TimeoutList, which is
// faster to locate the head and the tail of current list
struct TimeoutItem {
  int remaining_ticks_;        // the remaining ticks of current timeout event
  int repeat_interval_;        // the repeat interval of current timeout event
  uint64_t register_time_;     // the register timestamp of this timeout event
  int repeat_count_;           // the times of executing, -1 means infinity
  std::function<void()> func_; // the callback function of this timeout event

  std::shared_ptr<Coroutine>
      perform_co_; // coroutine executing this timeout event
};

class TimeWheel {
public:
  ~TimeWheel() = default;

  /**
   * @param slots the number of slot
   * @param granularity the granularity of each slot
   */
  TimeWheel(size_t slots = 1 * 1000, size_t granularity = 1);

  /**
   * @brief add timeout event to timewheel
   *
   * @attention co cannot pass main coroutine, since main coroutine always
   * running, its state is not CO_READY
   *
   * @attention if co is not nullptr and when timeout event is triggered, we
   * resume the execute of coroutine instead of execute func, which means that
   * in this scenario, func will be ignored
   *
   * @attention if user want to add timer event executing callback function, co
   * must be set to nullptr
   *
   * @param expired relative expire time, millisecond
   * @param func callback function of this timeout event
   * @param co executing coroutine
   * @param times the times of loop
   */
  void AddTimer(uint64_t expired, std::function<void()> func,
                std::shared_ptr<Coroutine> co = nullptr, int times = 1);

  void ExecuteTimeout(uint64_t now);

  void SetGranularity(size_t granularity) { granularity_ = granularity; }

  size_t GetGranularity() { return granularity_; }

  /**
   * @brief return interval ofthe next timeout event
   *
   * @return uint64_t the millisecond start from now and end to next timeout
   * event, zero means timewheel is empty
   */
  uint64_t GetNextTimeout(uint64_t now);

  std::string Dump();

protected:
  std::vector<std::list<std::shared_ptr<TimeoutItem>>> wheel_;
  size_t slots_;          // the size of wheel, default is 1 * 1000(which is 1s)
  size_t granularity_;    // the granularity of each slot
  uint64_t last_trigger_; // last trigger timestamp
  uint64_t current_slot;  // the index of iterating time wheel
};

} // namespace Sylar

#endif