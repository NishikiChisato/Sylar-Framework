#include "include/coroutine.h"
#include "include/schedule.h"

namespace Sylar {

static std::atomic<uint64_t> coroutine_id{0};
static std::atomic<uint64_t> coroutine_cnt{0};

thread_local Coroutine::ptr Coroutine::t_cur_coroutine = nullptr;

thread_local Coroutine::ptr Coroutine::t_main_coroutine = nullptr;

Coroutine::Coroutine(const std::function<void()> &f, uint64_t stack_size,
                     bool use_caller)
    : cid_(++coroutine_id), callback_(f), perform_schedule_(use_caller) {
  coroutine_cnt++;

  if (getcontext(&ctx_) == -1) {
    SYLAR_ASSERT_PREFIX(false, "getcontext");
  }

  stack_sz_ = stack_size ? stack_size : g_coroutine_stack_size->GetValue();
  stack_pt_ = MallocAlloc::Alloc(stack_sz_);

  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_pt_;
  ctx_.uc_stack.ss_size = stack_sz_;

  if (perform_schedule_) {
    Schedule::t_global_coroutine = std::shared_ptr<Coroutine>(new Coroutine());
    makecontext(&ctx_, &Coroutine::MainFunc, 0);
  } else {
    makecontext(&ctx_, &Coroutine::MainFunc, 0);
  }
}

Coroutine::Coroutine() {
  coroutine_cnt++;
  cid_ = ++coroutine_id;
  state_ = EXEC;
  if (getcontext(&ctx_) == -1) {
    SYLAR_ASSERT_PREFIX(false, "getcontext");
  }
}

Coroutine::~Coroutine() {
  if (stack_pt_) {
    // sub-coroutine
    SYLAR_ASSERT(state_ == INIT || state_ == READY || state_ == TERM ||
                 state_ == EXCEPT)
    MallocAlloc::DeAlloc(stack_pt_);
  } else {
    // main coroutine
    SYLAR_ASSERT(state_ == EXEC);
    SYLAR_ASSERT(!callback_);
    t_cur_coroutine = nullptr;
    t_main_coroutine = nullptr;
  }
  coroutine_cnt--;
}

void Coroutine::Reset(const std::function<void()> &f) {
  SYLAR_ASSERT(state_ == TERM || state_ == TERM || state_ == INIT);
  SYLAR_ASSERT(stack_pt_);

  if (getcontext(&ctx_) == -1) {
    SYLAR_ASSERT_PREFIX(false, "getcontext");
  }

  callback_ = f;

  stack_pt_ = MallocAlloc::Alloc(stack_sz_);
  ctx_.uc_link = &t_main_coroutine->ctx_;
  ctx_.uc_stack.ss_sp = &stack_pt_;
  ctx_.uc_stack.ss_size = stack_sz_;

  makecontext(&ctx_, &Coroutine::MainFunc, 0);
  state_ = READY;
}

void Coroutine::SwapIn() {
  SetThis(shared_from_this());
  state_ = EXEC;
  if (perform_schedule_) {
    if (swapcontext(&Schedule::GetGlobalCo()->ctx_, &ctx_) == -1) {
      SYLAR_ASSERT_PREFIX(false, "swapcontext");
    }
  } else {
    if (swapcontext(&t_main_coroutine->ctx_, &ctx_) == -1) {
      SYLAR_ASSERT_PREFIX(false, "swapcontext");
    }
  }
}

void Coroutine::SwapOut() {
  SetThis(shared_from_this());
  if (perform_schedule_) {
    if (swapcontext(&ctx_, &Schedule::GetGlobalCo()->ctx_) == -1) {
      SYLAR_ASSERT_PREFIX(false, "swapcontext");
    }
  } else {
    if (swapcontext(&ctx_, &t_main_coroutine->ctx_) == -1) {
      SYLAR_ASSERT_PREFIX(false, "swapcontext");
    }
  }
}

void Coroutine::Resume() { SwapIn(); }

void Coroutine::YieldToHold() {
  auto cur = GetThis();
  SYLAR_ASSERT(cur);
  cur->state_ = HOLD;
  cur->SwapOut();
}

void Coroutine::YieldToReady() {
  auto cur = GetThis();
  SYLAR_ASSERT(cur);
  cur->state_ = READY;
  cur->SwapOut();
}

void Coroutine::SetThis(Coroutine::ptr ptr) { t_cur_coroutine = ptr; }

Coroutine::ptr Coroutine::GetThis() {
  if (t_cur_coroutine) {
    return t_cur_coroutine->shared_from_this();
  }
  return t_main_coroutine->shared_from_this();
}

Coroutine::ptr Coroutine::CreateMainCo() {
  if (!t_main_coroutine) {
    t_main_coroutine = std::shared_ptr<Coroutine>(new Coroutine());
  }
  return t_main_coroutine;
}

uint64_t Coroutine::TotalCoroutine() { return coroutine_cnt; }

uint64_t Coroutine::GetCoroutineId() {
  if (t_main_coroutine) {
    return t_cur_coroutine->cid_;
  }
  return 0;
}

void Coroutine::MainFunc() {
  auto cur = GetThis();
  SYLAR_ASSERT(cur && cur->callback_);
  try {
    cur->callback_();
    cur->callback_ = nullptr;
    cur->state_ = TERM;
  } catch (...) {
    cur->state_ = EXCEPT;
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT) << "current coroutine occur exception";
  }

  auto ptr = cur.get();
  // sub-coroutine now should be managed by three pointer: current coroutine
  // pointer object, t_cur_coroutine and cur. we must release cur; otherwise
  // deconstructor will no execute
  cur.reset();
  // in main coroutine, we might call SwapIn multiple times, callback_ might
  // execute end, so  we should manually SwapOut
  ptr->SwapOut();
}

} // namespace Sylar
