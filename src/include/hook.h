#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__

#include "./fdcontext.h"
#include "./iomanager.h"
#include <dlfcn.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>
#include <utility>

namespace Sylar {
bool GetHookEnable();
void SetHookEnable(bool val);
} // namespace Sylar

extern "C" {

// sleep function
typedef unsigned int (*sleep_func)(unsigned int);
extern sleep_func sleep_f;

typedef int (*usleep_func)(useconds_t);
extern usleep_func usleep_f;

// read function
typedef ssize_t (*read_func)(int, void *, size_t);
extern read_func read_f;

typedef ssize_t (*readv_func)(int, const struct iovec *, int);
extern readv_func readv_f;

typedef ssize_t (*recv_func)(int, void *, size_t, int);
extern recv_func recv_f;

typedef ssize_t (*recvfrom_func)(int, void *, size_t, int, struct sockaddr *,
                                 socklen_t *);
extern recvfrom_func recvfrom_f;

typedef ssize_t (*recvmsg_func)(int, struct msghdr *, int);
extern recvmsg_func recvmsg_f;

// write function
typedef ssize_t (*write_func)(int, const void *, size_t);
extern write_func write_f;

typedef ssize_t (*writev_func)(int, const struct iovec *, int);
extern writev_func writev_f;

typedef ssize_t (*send_func)(int, const void *, size_t, int);
extern send_func send_f;

typedef ssize_t (*sendto_func)(int, const void *, size_t, int,
                               const struct sockaddr *, socklen_t);
extern sendto_func sendto_f;

typedef ssize_t (*sendmsg_func)(int, const struct msghdr *, int);
extern sendmsg_func sendmsg_f;

// close file
typedef int (*close_func)(int);
extern close_func close_f;
}

#endif