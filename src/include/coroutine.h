#ifndef __SYLAR_COROUTINE_H__
#define __SYLAR_COROUTINE_H__

#include "thread.h"
#include <functional>
#include <memory>
#include <ucontext.h>

namespace Sylar {

class Coroutine : std::enable_shared_from_this<Coroutine> {
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

private:
  /**
   * we should let default constructor is private, public constructor must pass
   * parameter
   */
  Coroutine();

  uint64_t coroutine_id_;          // coroutine id
  uint64_t stack_size_ = 0;        // the size of stack of coroutine
  State state_ = INIT;             // the state of coroutine
  ucontext_t ctx_;                 // the context of coroutine
  void *stack_pt_ = nullptr;       // the pointer of current stack
  std::function<void()> callback_; // callback function
};

} // namespace Sylar

#endif