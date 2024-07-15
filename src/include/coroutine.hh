#ifndef __SYLAR_COROUTINE_H__
#define __SYLAR_COROUTINE_H__

#include "util.hh"
#include <atomic>
#include <cstring>
#include <functional>
#include <memory>
#include <ucontext.h>
#include <vector>

namespace Sylar {

class MemoryAlloc {
public:
  static std::shared_ptr<void> AllocSharedMemory(size_t size) {
    std::shared_ptr<void> ptr((malloc(size)), free);
    if (!ptr) {
      throw std::bad_alloc();
    }
    return ptr;
  }

  static std::unique_ptr<void, decltype(&free)> AllocUniqueMemory(size_t size) {
    std::unique_ptr<void, decltype(&free)> ptr((malloc(size)), free);
    if (!ptr) {
      throw std::bad_alloc();
    }
    return ptr;
  }
};

class Coroutine;

struct StackMem_t {
  // this stack memory is occupied by which coroutine, this field only used in
  // shared memory. in this scenario, this stack memory may be shared by
  // multiply coroutine, we should save this stack memory to previous
  // coroutine's space and resume this stack memory from next coroutine's space
  std::shared_ptr<Coroutine> occupy_co_;
  // the size of stack memory
  size_t size_;
  // the start point of stack memory
  // the end point of stack memory, which is equal to
  // stack_buffer_ + size_
  std::shared_ptr<void> stack_buffer_;

  static std::shared_ptr<StackMem_t> AllocStack(size_t size) {
    std::shared_ptr<StackMem_t> ptr(new StackMem_t());
    ptr->size_ = size;
    ptr->occupy_co_ = nullptr;
    ptr->stack_buffer_ = MemoryAlloc::AllocUniqueMemory(size);
    return ptr;
  }
};

struct SharedMem_t {
  // the number of coroutine shares this shared stack memory
  size_t count_;
  // the size of each stack memory
  size_t size_;
  // shared memory consist of an array of stack memory, each stack memory can be
  // used by one coroutine
  std::vector<std::shared_ptr<StackMem_t>> stack_array_;
  // used to find empty stack memory in stack_array_
  size_t idx_;

  /**
   * @param count the number of coroutine will share this shared memory
   * @param size the size of each stack memory in shared memory
   */
  static std::shared_ptr<SharedMem_t> AllocSharedMem(size_t count,
                                                     size_t size) {
    auto ptr = std::make_shared<SharedMem_t>();
    ptr->count_ = count;
    ptr->size_ = size;
    ptr->idx_ = 0;
    ptr->stack_array_.resize(count);
    for (size_t i = 0; i < count; i++) {
      ptr->stack_array_[i] = StackMem_t::AllocStack(size);
    }
    return ptr;
  }

  static std::shared_ptr<StackMem_t>
  GetStackMem(std::shared_ptr<SharedMem_t> ptr) {
    if (!ptr) {
      return nullptr;
    }
    size_t i = ptr->idx_ % ptr->count_;
    ptr->idx_++;
    return ptr->stack_array_[i];
  }
};

struct CoroutineAttr {
  size_t stack_size_;
  std::shared_ptr<SharedMem_t> shared_mem_;

  CoroutineAttr() {
    stack_size_ = 64 * 1024;
    shared_mem_ = nullptr;
  }
};

class Schedule {
public:
  friend Coroutine;

  /**
   * when shared_ptr try to delete a object, it will invoke destructor.
   * if we declare destructor as inprivate, shared_ptr will not work.
   * either we declare destructor as public or privide custom private deletor
   * and delcare it as friend
   */
  ~Schedule() = default;

  static std::shared_ptr<Schedule> Instance() {
    static thread_local std::shared_ptr<Schedule> instance(new Schedule());
    return instance;
  }

  static std::shared_ptr<Schedule> GetThreadSchedule() { return t_schedule_; }

  static void InitThreadSchedule();

  static void Yield();

  // private:
  static void SwapContext(std::shared_ptr<Coroutine> prev,
                          std::shared_ptr<Coroutine> next);

  Schedule() = default;

  // the execute stack of coroutine. main coroutine always in stack buttom
  std::vector<std::shared_ptr<Coroutine>> co_stack_;
  // the stack pointer, always point to the top of stack
  size_t stack_top_;
  // the running coroutine
  std::shared_ptr<Coroutine> running_co_;

  static thread_local std::shared_ptr<Schedule> t_schedule_;
};

class Coroutine : public std::enable_shared_from_this<Coroutine> {
public:
  friend Schedule;

  enum CoState {
    CO_READY = 0,
    CO_RUNNING = 1,
    CO_TERMINAL = 2,
  };

  struct Deletor {
    void operator()(Coroutine *ptr) {
      assert(ptr->co_state_ == CO_TERMINAL);
      std::cout << "Deletro execute" << std::endl;
      free(ptr);
    }
  };

  static std::shared_ptr<Coroutine>
  CreateCoroutine(std::shared_ptr<Schedule> sc,
                  std::shared_ptr<CoroutineAttr> attr,
                  std::function<void()> func);

  void Resume();

  CoState GetCoState() { return co_state_; }

private:
  static void CoMainFunc(void *ptr);

  void SaveStack();
  void ResumeStack();

  Coroutine() = default;
  ~Coroutine() = default;

  std::shared_ptr<Schedule> schedule_; // equivalent to execute environment
  std::function<void()> func_;         // the execute function of coroutine
  ucontext_t coctx_;                   // context used for swapping
  CoState co_state_;                   // the state of coroutine
  bool is_main_co_;                    // whether main coroutine or not
  bool use_shared_stk_;                // whether use shared stack or not

  std::shared_ptr<StackMem_t>
      stack_mem_; // the execute memory of current coroutine

  std::shared_ptr<void> saved_stack_; // used to save execute stack space
  size_t saved_size_;                 // the size of saved stack space
  void *dummy_; // the effect of this field, plese check out coroutine.cc
};

} // namespace Sylar

#endif