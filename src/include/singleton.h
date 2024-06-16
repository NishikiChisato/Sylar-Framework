#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

#include <iostream>
#include <memory>

namespace Sylar {

// return raw pointer
template <typename T, typename P = void, int N = 0> class Singleton {
public:
  static T *GetInstance() {
    static T instance;
    return &instance;
  }

private:
  Singleton() {}
  ~Singleton() {}
  Singleton(const Singleton &);
  Singleton(const Singleton &&);
  Singleton &operator=(const Singleton &);
};

// return smart pointer
template <typename T, typename P = void, int N = 0> class SingletonPtr {
public:
  static std::shared_ptr<T> &GetInstance() {
    static std::shared_ptr<T> instance(new T);
    return instance;
  }

private:
  SingletonPtr() {}
  ~SingletonPtr() {}
  SingletonPtr(const SingletonPtr &);
  SingletonPtr(const SingletonPtr &&);
  SingletonPtr &operator=(const SingletonPtr &);
};

} // namespace Sylar

#endif