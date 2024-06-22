#include "include/thread.h"
#include "include/log.h"
#include "util.cc"

namespace Sylar {

// Each thread has these two variable. When thread begins, the variable is
// allocated and when thread ends, the variable is deallocated
static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

Thread *Thread::GetThis() { return t_thread; }
std::string Thread::GetName() { return t_thread_name; }
void Thread::SetName(const std::string &name) {
  if (t_thread) {
    t_thread->name_ = name;
  }
  t_thread_name = name;
}

Thread::Thread(std::function<void()> &f, const std::string &name)
    : callback_(f), name_(name), sem_() {
  int rt = pthread_create(&thread_, nullptr, &Thread::run, this);
  if (rt) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT) << "pthread_create error with rt: " << rt
                                    << ", name: " << name << std::endl;
    std::logic_error("Thread::Thread pthread_create error");
  }

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
        << "pthread_create error with rt: " << rt << std::endl;
    std::logic_error("Thread::Join pthread_join error");
  }
}

void *Thread::run(void *arg) {
  Thread *thread = static_cast<Thread *>(arg);
  t_thread = thread;
  t_thread->tid_ = Sylar::GetThreadId();
  t_thread_name = t_thread->GetName();

  std::function<void()> f;
  f.swap(t_thread->callback_);
  t_thread->sem_.Notify();
  f();
  return nullptr;
}

} // namespace Sylar
