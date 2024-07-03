#include "include/schedule.h"
#include "include/coroutine.h"
#include "include/mutex.h"
#include "include/thread.h"

namespace Sylar {

thread_local Schedule *Schedule::t_schedule = nullptr;
thread_local Coroutine::ptr Schedule::t_global_coroutine = nullptr;

Schedule::Schedule(size_t threads, bool use_caller, const std::string &name)
    : name_(name) {
  SYLAR_ASSERT(threads > 0);
  if (use_caller) {
    threads--;
    // create a new coroutine to schedule task
    Coroutine::CreateMainCo();
    root_coroutine_ = std::make_shared<Coroutine>(
        std::bind(&Schedule::Run, this), 64 * 1024, true);
    root_thread_id_ = GetThreadId();

    t_schedule = this;
  } else {
    root_thread_id_ = -1;
  }
  thread_num_ = threads;
}

Schedule::~Schedule() {
  t_schedule = nullptr;
  t_global_coroutine = nullptr;
}

void Schedule::Start() {
  Mutex::ScopeLock l(mu_);
  if (!is_stoped_) {
    return;
  }
  is_stoped_ = false;
  SYLAR_ASSERT(threads_.empty());
  threads_.resize(thread_num_);
  for (int i = 0; i < thread_num_; i++) {
    // threads_[i] = std::make_shared<Thread>(std::bind(&Schedule::Run, this),
    // std::string("thread_") + std::to_string(i));
    // thread_ids_.push_back(threads_[i]->GetThreadId());
    threads_[i] =
        std::make_shared<std::thread>(std::bind(&Schedule::Run, this));
    thread_ids_.push_back(threads_[i]->get_id());
  }
}

void Schedule::Stop() {
  is_stoped_ = true;
  // only current thread perform schedule and execute
  if (root_coroutine_ && thread_num_ == 0 &&
      (root_coroutine_->GetState() == Coroutine::INIT ||
       root_coroutine_->GetState() == Coroutine::TERM)) {

    if (IsStop()) {
      return;
    }
  }

  // we resume once root coroutine to guarantee that root coroutine has exited
  if (root_coroutine_) {
    root_coroutine_->Resume();
  }

  // wait for each thread to exit, it will block in this place
  std::vector<std::shared_ptr<std::thread>> vthrs;
  {
    Mutex::ScopeLock l(mu_);
    vthrs.swap(threads_);
  }
  for (auto &i : vthrs) {
    if (i->joinable()) {
      i->join();
    }
  }
}

void Schedule::Run() {
  Coroutine::ptr co_task;
  CoroutineTask ct;
  while (!IsStop()) {
    ct.Reset();
    Coroutine::CreateMainCo();
    Coroutine::ptr idel_co(
        new Coroutine(std::bind(&Schedule::Idel, this), 1024 * 1024, false));

    {
      Mutex::ScopeLock l(mu_);
      auto it = tasks_.begin();
      while (it != tasks_.end()) {
        // if this task specifies thread id
        if (it->thread_id_ != -1 && it->thread_id_ != GetThreadId()) {
          it++;
          continue;
        }
        SYLAR_ASSERT(it->coroutine_ || it->f_);
        if (it->coroutine_) {
          SYLAR_ASSERT(it->coroutine_->GetState() == Coroutine::READY);
        }
        ct = *it;
        tasks_.erase(it);
        active_thread_num_++;
        break;
      }
    }

    if (ct.coroutine_) {
      ct.coroutine_->Resume();
      active_thread_num_--;
      ct.Reset();
    } else if (ct.f_) {
      if (co_task) {
        co_task->Reset(ct.f_);
      } else {
        co_task = std::make_shared<Coroutine>(ct.f_, 1024 * 1024, false);
      }
      co_task->Resume();
      active_thread_num_--;
      ct.Reset();
      co_task.reset();
    } else {
      // task has run out, this coroutine should perform idel method
      idel_thread_num_++;
      idel_co->Resume();
      idel_thread_num_--;
    }
  }
}

bool Schedule::IsStop() {
  return tasks_.empty() && active_thread_num_ == 0 && is_stoped_;
}

void Schedule::Idel() {
  while (!IsStop()) {
    Sylar::Coroutine::YieldToHold();
  }
}

} // namespace Sylar