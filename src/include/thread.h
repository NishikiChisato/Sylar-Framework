#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include "mutex.h"
#include <boost/noncopyable.hpp>
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <thread>

namespace Sylar {

class Thread : public boost::noncopyable {
public:
  typedef std::shared_ptr<Thread> ptr;
  Thread(const std::function<void()> &f, const std::string &name);
  ~Thread();

  /**
   * these three methods should be declare as static. the reason is that a
   * static method can be called without an instance of object, we can use these
   * methods during thread execution
   */
  static Thread *GetThis();
  static std::string GetName();
  static void SetName(const std::string &name);

  void Join();

private:
  pid_t tid_ = -1;                 // thread id
  pthread_t thread_ = 0;           // the struct of thread, not thread id
  std::function<void()> callback_; // the function of processing by this thread
  std::string name_;               // thread name
  Semaphore sem_;                  // semaphore

  /**
   * keyword explain:
   * thread_local: guarantee that each thread has its own local instance of
   * t_thread and t_thread_name, which prevent data from being shared between
   * threads
   *
   * static: provite static storage duration, meaning that this variable will be
   * created when thread starts and destoried when thread exits
   *
   * why do we use this design:
   * during thread execution, it can provide an easy way to access thread data
   * without passing pointer or reference explicitly
   */
  static thread_local Thread *t_thread;
  static thread_local std::string t_thread_name;

  /**
   * this function will acturally execute callback_
   * because we must provide a way to get information about this thread in
   * thread execution(in callback function)
   *
   * so, we must declare GetThis()/GetName()/SetName() as static, these three
   * function can be called in callback_ function
   *
   * these function is designed for initialize for t_thread and t_thread_name
   */
  static void *run(void *arg);
};

} // namespace Sylar

#endif