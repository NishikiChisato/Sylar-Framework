#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include <atomic>
#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <semaphore.h>
#include <thread>
#include <unistd.h>

namespace Sylar {

class Semaphore : public boost::noncopyable {
public:
  Semaphore(uint32_t cnt = 1);
  ~Semaphore();
  void Wait();
  void Notify();

private:
  sem_t sem_;
};

template <typename T> class ScopeLockImpl {
public:
  ScopeLockImpl(T &mu) : mu_(mu) {
    mu_.Lock();
    is_locked_ = true;
  }
  ~ScopeLockImpl() { Unlock(); }
  void Lock() {
    if (!is_locked_) {
      mu_.Lock();
      is_locked_ = true;
    }
  }
  void Unlock() {
    if (is_locked_) {
      mu_.Unlock();
      is_locked_ = false;
    }
  }

private:
  T &mu_;
  bool is_locked_;
};

class Mutex : boost::noncopyable {
public:
  typedef ScopeLockImpl<Mutex> ScopeLock;
  Mutex() { pthread_mutex_init(&mu_, nullptr); }
  ~Mutex() { pthread_mutex_destroy(&mu_); }
  void Lock() { pthread_mutex_lock(&mu_); }
  void Unlock() { pthread_mutex_unlock(&mu_); }

private:
  pthread_mutex_t mu_;
};

class NullMutex : boost::noncopyable {
public:
  typedef ScopeLockImpl<NullMutex> ScopeLock;
  NullMutex() {}
  ~NullMutex() {}
  void Lock() {}
  void Unlock() {}
};

class SpinLock : boost::noncopyable {
public:
  typedef ScopeLockImpl<SpinLock> ScopeLock;
  SpinLock() { pthread_spin_init(&mu_, 0); }
  ~SpinLock() { pthread_spin_destroy(&mu_); }
  void Lock() { pthread_spin_lock(&mu_); }
  void Unlock() { pthread_spin_unlock(&mu_); }

private:
  pthread_spinlock_t mu_;
};

// compare and swap(CAS)
class CASLock : boost::noncopyable {
public:
  typedef ScopeLockImpl<CASLock> ScopeLock;
  CASLock() { mu_.clear(); }
  ~CASLock() {}
  void Lock() {
    // set mu_ to true and return the value before
    // if the value before is false, Lock() will set it to true
    // else if the value before is true, Lock() will spin untill the value be
    // false
    while (
        std::atomic_flag_test_and_set_explicit(&mu_, std::memory_order_acquire))
      ;
  }
  void Unlock() {
    std::atomic_flag_clear_explicit(&mu_, std::memory_order_release);
  }

private:
  volatile std::atomic_flag mu_;
};

template <typename T> class ReadScopeLockImpl {
public:
  ReadScopeLockImpl(T &mu) : mu_(mu) {
    mu_.Lock();
    is_locked_ = true;
  }
  ~ReadScopeLockImpl() { Unlock(); }
  void Lock() {
    if (!is_locked_) {
      mu_.Rdlock();
      is_locked_ = true;
    }
  }
  void Unlock() {
    if (is_locked_) {
      mu_.Unlock();
      is_locked_ = false;
    }
  }

private:
  T &mu_;
  bool is_locked_;
};

template <typename T> class WriteScopeLockImpl {
public:
  WriteScopeLockImpl(T &mu) : mu_(mu) {
    mu_.Wrlock();
    is_locked_ = true;
  }
  ~WriteScopeLockImpl() { Unlock(); }
  void Lock() {
    if (!is_locked_) {
      mu_.Wrlock();
      is_locked_ = true;
    }
  }
  void Unlock() {
    if (is_locked_) {
      mu_.Unlock();
      is_locked_ = false;
    }
  }

private:
  T &mu_;
  bool is_locked_;
};

class RWMutex : boost::noncopyable {
public:
  typedef ReadScopeLockImpl<RWMutex> ReadLock;
  typedef WriteScopeLockImpl<RWMutex> WriteLock;
  RWMutex() { pthread_rwlock_init(&mu_, nullptr); }
  ~RWMutex() { pthread_rwlock_destroy(&mu_); }
  void Rdlock() { pthread_rwlock_rdlock(&mu_); }
  void Wrlock() { pthread_rwlock_wrlock(&mu_); }
  void Unlock() { pthread_rwlock_unlock(&mu_); }

private:
  pthread_rwlock_t mu_;
};

class NullRWMutex : boost::noncopyable {
public:
  typedef ReadScopeLockImpl<NullRWMutex> ReadLock;
  typedef WriteScopeLockImpl<NullRWMutex> WriteLock;
  NullRWMutex() {}
  ~NullRWMutex() {}
  void Rdlock() {}
  void Wrlock() {}
  void Unlock() {}
};

} // namespace Sylar

#endif