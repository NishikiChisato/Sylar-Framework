#include "include/thread.h"
#include "include/log.h"
#include "include/util.h"

namespace Sylar {

thread_local Thread *Thread::t_thread = nullptr;
thread_local std::string Thread::t_thread_name = "UNKNOW";

Thread *Thread::GetThis() { return t_thread; }
std::string Thread::GetName() { return t_thread_name; }
void Thread::SetName(const std::string &name) {
  if (t_thread) {
    t_thread->name_ = name;
  }
  t_thread_name = name;
}

Thread::Thread(const std::function<void()> &f, const std::string &name)
    : callback_(f), name_(name), sem_() {

  int rt = pthread_create(&thread_, nullptr, &Thread::run, this);

  if (rt) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT) << "pthread_create error with rt: " << rt
                                    << ", name: " << name << std::endl;
    std::logic_error("Thread::Thread pthread_create error");
  }

  // we should guarantee that after this thread object is fully initialize, we
  // can execute f(function)
  sem_.Wait();
}

Thread::~Thread() {
  if (thread_) {
    pthread_detach(thread_);
  }
}

void Thread::Join() {
  int rt = pthread_join(thread_, nullptr);
  if (rt) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "pthread_join error with rt: " << rt << std::endl;
    std::logic_error("Thread::Join pthread_join error");
  }
}

/**
 * @brief this function is subthread
 * t_thread will exist during running of this function
 *
 * @param arg is the object of subthread, we should assign this object to
 * t_thread
 */

void *Thread::run(void *arg) {
  Thread *thread = static_cast<Thread *>(arg);
  t_thread = thread;
  t_thread->tid_ = Sylar::GetThreadId();
  t_thread_name = t_thread->name_;

  std::function<void()> f;
  f.swap(t_thread->callback_);
  t_thread->sem_.Notify();
  f();
  return nullptr;
}

} // namespace Sylar
