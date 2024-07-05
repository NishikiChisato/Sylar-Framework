#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <assert.h>
#include <execinfo.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace Sylar {

#define SYLAR_ASSERT(x)                                                        \
  if (!(x)) {                                                                  \
    std::cout << "assert error: " #x << Sylar::BacktraceToString(100, 2);      \
    assert(x);                                                                 \
  }

#define SYLAR_ASSERT_PREFIX(x, prefix)                                         \
  if (!(x)) {                                                                  \
    std::cout << "assert error: " #x                                           \
              << Sylar::BacktraceToString(100, 2, prefix);                     \
    assert(x);                                                                 \
  }

// return process id
pid_t GetProcessId();

// return thread id
pid_t GetThreadId();

// get time from system reboot
uint64_t GetElapseFromRebootMS();

void BackTrace(std::vector<std::string> &bt, int sz, int skip);

std::string BacktraceToString(int sz, int skip, const std::string &prefix = "");

} // namespace Sylar

#endif