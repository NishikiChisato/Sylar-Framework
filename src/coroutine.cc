#include "include/coroutine.hh"
#include "include/epoll.hh"
#include "include/timewheel.hh"

namespace Sylar {

thread_local std::shared_ptr<Schedule> Schedule::t_schedule_ = nullptr;

thread_local std::shared_ptr<Epoll> Schedule::t_epoll_ = nullptr;

void Schedule::InitThreadSchedule() {
  t_schedule_ = Schedule::Instance();
  auto main_co = Coroutine::CreateCoroutine(t_schedule_, nullptr, nullptr);
  main_co->is_main_co_ = true;
  main_co->co_state_ = Coroutine::CO_RUNNING;
  // we should init context of main_co in this place
  t_schedule_->co_stack_.push_back(main_co);
  t_schedule_->stack_top_ = 1;
  t_schedule_->running_co_ = main_co;
}

std::shared_ptr<Coroutine> Schedule::GetCurrentCo() {
  if (!GetThreadSchedule()) {
    InitThreadSchedule();
  }
  auto schedule = Schedule::GetThreadSchedule();
  auto top = schedule->co_stack_.back();
  return top;
}

void Schedule::Yield() {
  auto t_schedule = Schedule::Instance();
  if (t_schedule->co_stack_.size() < 2) {
    throw std::runtime_error("only main coroutine running, cannot yield");
  }
  auto running_co = t_schedule->co_stack_[t_schedule->stack_top_ - 1];
  auto pending_co = t_schedule->co_stack_[t_schedule->stack_top_ - 2];
  t_schedule->co_stack_.pop_back();
  t_schedule->stack_top_--;

  t_schedule->running_co_ = pending_co;

  running_co->co_state_ = Coroutine::CO_READY;
  pending_co->co_state_ = Coroutine::CO_RUNNING;
  SwapContext(running_co, pending_co);
}

void Schedule::SwapContext(std::shared_ptr<Coroutine> prev,
                           std::shared_ptr<Coroutine> next) {
  // if the next coroutine use shared memroy, its stack memory may be used by
  // another coroutine, we should save this stack memory to that coroutine. we
  // always save the stack of next coroutine.
  // main coroutine will not use shared memory, so when we enter this function,
  // we must place in stack space of coroutine, which is place in heap space in
  // all program space

  // save current stack top
  char dummy;
  prev->dummy_ = &dummy;

  if (next->use_shared_stk_) {
    // save the previous coroutine occupied in next->stack_mem_
    auto occupy_co = next->stack_mem_->occupy_co_;
    next->stack_mem_->occupy_co_ = next;
    if (occupy_co && occupy_co != next) {
      occupy_co->SaveStack();
      // resume the stack memory saved in next
      next->ResumeStack();
    }
  }

  swapcontext(&prev->coctx_, &next->coctx_);
}

void Schedule::InitThreadEpoll() {
  t_epoll_ = Epoll::Instance();
  Epoll::InitEpoll(t_epoll_);
}

void Schedule::Eventloop(std::shared_ptr<Epoll> epoll) {
  const uint64_t MAX_TIMEOUT = 1000;
  const int MAX_EVENT = 256;
  epoll_event *evs = (epoll_event *)calloc(MAX_EVENT, sizeof(epoll_event));
  std::shared_ptr<epoll_event> deleter(evs, free);
  while (!epoll->Stoped()) {
    // we can set granularity in epoll
    uint64_t now = GetElapseFromRebootMS();
    uint64_t interval = epoll->GetNextTimeout(now);
    uint64_t timeout =
        (interval == 0 ? MAX_TIMEOUT : std::min(interval + 1, MAX_TIMEOUT));
    int cnt = epoll_wait(epoll->epfd_, evs, MAX_EVENT, timeout);
    if (cnt <= 0) {
      if (errno == EAGAIN) {
        std::cout << "epoll_wait timeout" << std::endl;
      }
    }
    for (int i = 0; i < cnt; i++) {
      auto &ev = evs[i];
      Epoll::EventCtx *ptr = (Epoll::EventCtx *)ev.data.ptr;

      if (ev.events & EPOLLIN) {
        if (ptr->r_callback_) {
          ptr->r_callback_();
        } else if (ptr->r_co_) {
          ptr->r_co_->Resume();
        }
      }
      if (ev.events & EPOLLOUT) {
        if (ptr->w_callback_) {
          ptr->w_callback_();
        } else if (ptr->w_co_) {
          ptr->w_co_->Resume();
        }
      }
    }
    now = GetElapseFromRebootMS();
    epoll->ExecuteTimeout(now);
  }
}

void Coroutine::SaveStack() {

  // clang-format off
  //     heap space
  // |------------------|  (high address)
  // |                  | 
  // |                  | 
  // |==================| <- this->stack_mem_->stack_buffer_+this->stack_mem_->size_ 
  // |                  | 
  // |                  | 
  // |                  | 
  // |                  | the program execute stack always grow towards lower address
  // |==================| so, may be stack top of program is here, which is the address of this->dummy_
  // |                  | 
  // |                  | 
  // |                  | 
  // |                  | 
  // |                  | 
  // |                  | 
  // |                  | 
  // |                  | 
  // |                  | 
  // |==================| <- this->stack_mem_->stack_buffer_.get() 
  // |                  | 
  // |                  | 
  // |                  | 
  // |                  | 
  // |                  | 
  // |------------------|  (low address)
  // clang-format on

  size_t len = (char *)this->stack_mem_->stack_buffer_.get() +
               this->stack_mem_->size_ - (char *)this->dummy_;
  this->saved_stack_ = MemoryAlloc::AllocSharedMemory(len);
  this->saved_size_ = len;
  memcpy(this->saved_stack_.get(), this->dummy_, len);
}

void Coroutine::ResumeStack() {
  memcpy(this->dummy_, this->saved_stack_.get(), this->saved_size_);
}

std::shared_ptr<Coroutine>
Coroutine::CreateCoroutine(std::shared_ptr<Schedule> sc,
                           std::shared_ptr<CoroutineAttr> attr,
                           std::function<void()> func) {
  if (!Schedule::GetThreadSchedule()) {
    Schedule::InitThreadSchedule();
  }

  std::shared_ptr<Coroutine> co(new Coroutine(), Deletor());

  if (attr) {
    // let the last 12 bits to be zero
    if (attr->stack_size_ & 0xfff) {
      attr->stack_size_ &= ~0xfff;
      attr->stack_size_ += 0x1000;
    }
    if (attr->shared_mem_) {
      co->use_shared_stk_ = true;
      co->stack_mem_ = SharedMem::GetStackMem(attr->shared_mem_);
    } else {
      co->use_shared_stk_ = false;
      co->stack_mem_ = StackMem::AllocStack(attr->stack_size_);
    }
  }

  co->schedule_ = sc;
  co->is_main_co_ = false;
  co->co_state_ = CoState::CO_READY;
  co->saved_stack_ = nullptr;
  co->saved_size_ = 0;
  getcontext(&co->coctx_);

  if (attr) {
    co->func_.swap(func);
    co->coctx_.uc_stack.ss_sp = co->stack_mem_->stack_buffer_.get();
    co->coctx_.uc_stack.ss_size = co->stack_mem_->size_;
    co->coctx_.uc_link = nullptr;
    makecontext(&co->coctx_, (void (*)()) & Coroutine::CoMainFunc, 1,
                (void *)co.get());
  }

  return co;
}

void Coroutine::Resume() {
  auto t_schedule = schedule_->GetThreadSchedule();
  if (co_state_ == CO_TERMINAL) {
    return;
  }
  SYLAR_ASSERT(co_state_ == CO_READY);
  SYLAR_ASSERT(t_schedule->running_co_->co_state_ == CO_RUNNING);

  auto prev = t_schedule->running_co_;
  auto next = shared_from_this();
  if (t_schedule->co_stack_.size() > 1) {
    // we cannot change the state of main coroutine
    prev->co_state_ = CO_READY;
  }
  next->co_state_ = CO_RUNNING;
  t_schedule->co_stack_.push_back(next);
  t_schedule->stack_top_++;
  t_schedule->running_co_ = next;
  t_schedule->SwapContext(prev, next);
}

void Coroutine::CoMainFunc(void *ptr) {
  auto co = reinterpret_cast<Coroutine *>(ptr);
  co->func_();
  co->co_state_ = CO_TERMINAL;

  auto t_schedule = Schedule::Instance();
  auto running_co = t_schedule->co_stack_[t_schedule->stack_top_ - 1];
  auto pending_co = t_schedule->co_stack_[t_schedule->stack_top_ - 2];
  t_schedule->co_stack_.pop_back();
  t_schedule->stack_top_--;

  t_schedule->running_co_ = pending_co;
  t_schedule->SwapContext(running_co, pending_co);
}

} // namespace Sylar
