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
  Thread(std::function<void()> &f, const std::string &name);
  ~Thread();

  // these three function is static function, used by t_thread and
  // t_thread_name, instead of thread object
  static Thread *GetThis();
  static std::string GetName();
  static void SetName(const std::string &name);

  void Join();

private:
  static void *run(void *arg);

  pid_t tid_ = -1;                 // thread id
  pthread_t thread_ = 0;           // the struct of thread, not thread id
  std::function<void()> callback_; // the function of processing by this thread
  std::string name_;               // thread name
  Semaphore sem_;                  // semaphore
};

} // namespace Sylar

#endif