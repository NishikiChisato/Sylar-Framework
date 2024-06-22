#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace Sylar {

// return process id
pid_t GetProcessId() { return syscall(SYS_getpid); }

// return thread id
pid_t GetThreadId() { return syscall(SYS_gettid); }

} // namespace Sylar