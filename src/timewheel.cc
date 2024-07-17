#include "include/timewheel.hh"

namespace Sylar {

TimeWheel::TimeWheel(size_t slots, size_t granularity)
    : slots_(slots), granularity_(granularity), last_trigger_(0),
      current_slot(0) {
  wheel_.resize(slots_);
}

void TimeWheel::AddTimer(uint64_t timeout, std::function<void()> func,
                         std::shared_ptr<Coroutine> co, int times) {
  // last_trigger should replace to GetEpleaseFromRebootMS()
  auto item = std::make_shared<TimeoutItem>();
  item->remaining_ticks_ = (timeout / granularity_) / slots_;
  item->repeat_interval_ = (timeout / granularity_) / slots_;
  item->repeat_count_ = times;
  item->func_.swap(func);
  item->perform_co_ = co;

  auto slot = (current_slot + timeout / granularity_) % slots_;
  wheel_[slot].emplace_back(item);
}

void TimeWheel::ExecuteTimeout(uint64_t now) {
  if (now <= last_trigger_) {
    return;
  }
  auto dis = (now - last_trigger_) / granularity_;
  for (uint64_t i = 0; i < dis; i++) {
    auto &list = wheel_[current_slot];
    current_slot = (current_slot + 1) % slots_;
    for (auto it = list.begin(); it != list.end();) {
      auto &item = *it;
      item->remaining_ticks_--;
      if (item->remaining_ticks_ < 0) {
        item->func_();
        if (item->repeat_count_ > 0) {
          item->repeat_count_--;
        }
        if (item->repeat_count_ == 0) {
          it = list.erase(it);
        } else {
          item->remaining_ticks_ = item->repeat_interval_;
          auto new_slot = (current_slot + item->remaining_ticks_) % slots_;
          if (new_slot != current_slot) {
            wheel_[new_slot].splice(wheel_[new_slot].end(), list, it);
            it = list.begin();
          } else {
            it++;
          }
        }
      } else {
        it++;
      }
    }
  }
  last_trigger_ = now;
}

std::string TimeWheel::Dump() {
  std::stringstream ss;
  for (int i = 0; i < wheel_.size(); i++) {
    ss << "[" << i << "]:" << std::endl;
    for (auto it = wheel_[i].begin(); it != wheel_[i].end(); it++) {
      auto &item = *it;
      ss << "[remaining]: " << item->remaining_ticks_
         << ", [repeat]: " << item->repeat_count_ << std::endl;
    }
  }
  return ss.str();
}

} // namespace Sylar