#include "include/util.hh"

namespace Sylar {

// return process id
pid_t GetProcessId() { return syscall(SYS_getpid); }

// return thread id
pid_t GetThreadId() { return syscall(SYS_gettid); }

/**
 * @brief used to get backtrace information
 *
 * @param bt return vector, used to store string
 * @param sz the max size of stack
 * @param skip skip the number of stack
 */
void BackTrace(std::vector<std::string> &bt, int sz, int skip) {
  void **array = (void **)malloc(sizeof(void *) * sz);
  size_t s = ::backtrace(array, sz);
  char **str = ::backtrace_symbols(array, s);
  if (str == NULL) {
    std::cerr << "BackTrace error" << std::endl;
    free(array);
    free(str);
    return;
  }
  for (size_t i = skip; i < s; i++) {
    bt.push_back(str[i]);
  }
  free(array);
  free(str);
}

std::string BacktraceToString(int sz, int skip, const std::string &prefix) {
  std::vector<std::string> bt;
  BackTrace(bt, sz, skip);
  std::stringstream ss;
  for (int i = 0; i < bt.size(); i++) {
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}

uint64_t GetElapseFromRebootMS() {
  timespec ts{0};

  /**
   * A nonsettable system-wide clock that represents monotonic time since—as
   * described by POSIX—"some unspecified point in the past".  On Linux, that
   * point corresponds to the number of sec‐ onds that the system has been
   * running since it was booted.
   */

  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

} // namespace Sylar