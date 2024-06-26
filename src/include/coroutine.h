#ifndef __SYLAR_COROUTINE_H__
#define __SYLAR_COROUTINE_H__

#include "config.h"
#include "thread.h"
#include "util.h"
#include <atomic>
#include <functional>
#include <memory>
#include <ucontext.h>

namespace Sylar {

static ConfigVar<uint64_t>::ptr g_coroutine_stack_size =
    Config::Lookup("coroutine.stack.size", (uint64_t)1024 * 1024,
                   "the stack size of coroutine");

class MallocAlloc {
public:
  static void *Alloc(size_t size) {
    void *addr = malloc(size);
    return addr;
  }
  static void DeAlloc(void *ptr) { free(ptr); }
};

class Coroutine : public std::enable_shared_from_this<Coroutine> {
public:
  typedef std::shared_ptr<Coroutine> ptr;

  enum State {
    INIT,  //  initial state
    HOLD,  //  this coroutine is blocked and to execute another coroutine
    READY, //  this coroutine is ready to execute
    EXEC,  //  this coroutine is executing
    TERM,  //  this coroutine is terminal
    EXCEPT // this coroutine has uncertain exception
  };

public:
  Coroutine(const std::function<void()> &f, uint64_t stack_size = 0);

  /**
   * we should destory main coroutine and sub-coroutine. the former not has
   * stack pointer and the latter has stack pointer
   */
  ~Coroutine();

  /**
   * reset to these coroutine
   */
  void Reset(const std::function<void()> &f);

  /**
   * create main coroutine
   */
  static void CreateMainCo();

  void Resume();

  /**
   * used in user-defined function, used to yield the execution of current
   * function and swap to execution of main coroutine
   */
  static void YieldToReady();

  /**
   * identical to the previous one. the former set coroutine state to READY and
   * the latter set state to HOLD
   */
  static void YieldToHold();

  /**
   * used to get the number of coroutine
   */
  static uint64_t TotalCoroutine();

  /**
   * this function only can get the id of currently executing coroutine
   */
  static uint64_t GetCoroutineId();

private:
  /**
   * used by main coroutine, swap to execute sub-coroutine
   */
  void SwapIn();

  /**
   * used by sub-coroutine, yield to execution and swap to main coroutine
   */
  void SwapOut();

  /**
   *  used to assign value to t_cur_coroutine
   *
   * when user use parameterized constructor to create a new coroutine, that
   * coroutine, which executes user-defined function, is sub-coroutine and main
   * coroutine, which is current function ,is is not exist. we should use this
   * method to initialize main and sub coroutine
   *
   * all member function, including static member function, should call this
   * function
   */
  static void SetThis(Coroutine::ptr ptr);

  /**
   * used to get current coroutine, usually used in user-defined function
   */
  static Coroutine::ptr GetThis();

  /**
   * we should let default constructor is private, public constructor must pass
   * parameter
   *
   * this constructor is used to create main coroutine and parameterized
   * constructor is used to create sub-coroutine
   */
  Coroutine();

  /**
   * point to current executing coroutine. if current executing coroutine is
   * sub-coroutine, this pointer should point to sub-coroutine; if current
   * executing coroutine is main coroutine, this pointer should also point to
   * main coroutine
   */
  static thread_local Coroutine::ptr t_cur_coroutine;

  /**
   * always point to main coroutine
   */
  static thread_local Coroutine::ptr t_main_coroutine;

  /**
   * the main execute function of sub-coroutine
   */
  static void MainFunc();

  uint64_t cid_;                   // coroutine id
  State state_ = INIT;             // the state of coroutine
  ucontext_t ctx_;                 // the context of coroutine
  void *stack_pt_ = nullptr;       // the pointer of current stack
  uint64_t stack_sz_ = 0;          // the size of stack of coroutine
  std::function<void()> callback_; // callback function
};

} // namespace Sylar

#endif