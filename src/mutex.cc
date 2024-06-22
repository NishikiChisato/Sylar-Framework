#include "include/mutex.h"
#include <stdexcept>

namespace Sylar {
Semaphore::Semaphore(uint32_t cnt) {
  if (sem_init(&sem_, 0, cnt)) {
    std::logic_error("Semaphore::Semaphore error");
  }
}

Semaphore::~Semaphore() { sem_destroy(&sem_); }

void Semaphore::Wait() {
  if (sem_wait(&sem_)) {
    std::logic_error("Semaphore::Wait error");
  }
}

void Semaphore::Notify() {
  if (sem_post(&sem_)) {
    std::logic_error("Semaphore::Notify error");
  }
}

} // namespace Sylar