#include "include/coroutine.hh"

namespace Sylar {

thread_local std::shared_ptr<Schedule> Schedule::t_schedule_ = nullptr;

void PrintCoInvokeStack(Schedule *ptr) {
  for (auto &it : ptr->co_stack_) {
    switch (it->GetCoState()) {
    case Coroutine::CO_READY:
      std::cout << "CO_READY" << std::endl;
      break;
    case Coroutine::CO_RUNNING:
      std::cout << "CO_RUNNING" << std::endl;
      break;
    case Coroutine::CO_TERMINAL:
      std::cout << "CO_TERMINAL" << std::endl;
      break;
    }
  }
}

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
  // |                 | 
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
      co->stack_mem_ = SharedMem_t::GetStackMem(attr->shared_mem_);
    } else {
      co->use_shared_stk_ = false;
      co->stack_mem_ = StackMem_t::AllocStack(attr->stack_size_);
    }
  }

  co->schedule_ = sc;
  co->is_main_co_ = false;
  co->co_state_ = CoState::CO_READY;
  co->saved_stack_ = nullptr;
  co->saved_size_ = 0;

  if (attr) {
    co->func_.swap(func);
    getcontext(&co->coctx_);
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
  assert(co_state_ == CO_READY);
  assert(t_schedule->running_co_->co_state_ == CO_RUNNING);

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
