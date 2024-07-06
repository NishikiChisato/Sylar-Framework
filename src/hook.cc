#include "include/hook.h"

static thread_local bool is_hook_enable = true;

namespace Sylar {
bool GetHookEnable() { return is_hook_enable; }
void SetHookEnable(bool val) { is_hook_enable = val; }

#define HOOK_FUNC(XX)                                                          \
  XX(sleep)                                                                    \
  XX(usleep)                                                                   \
  XX(nanosleep)                                                                \
  XX(socket)                                                                   \
  XX(connect)                                                                  \
  XX(accept)                                                                   \
  XX(read)                                                                     \
  XX(readv)                                                                    \
  XX(recv)                                                                     \
  XX(recvfrom)                                                                 \
  XX(recvmsg)                                                                  \
  XX(write)                                                                    \
  XX(writev)                                                                   \
  XX(send)                                                                     \
  XX(sendto)                                                                   \
  XX(sendmsg)                                                                  \
  XX(close)

void hook_init() {
  static bool is_init = false;
  if (is_init) {
    return;
  }
#define XX(name) name##_func name##_f = (name##_func)dlsym(RTLD_NEXT, #name);
  HOOK_FUNC(XX)
#undef XX
}

struct _HookIniter {
  _HookIniter() { hook_init(); }
};

// a variable declared as static will initialize before execcution of main
// function
static _HookIniter _hook_initer;

} // namespace Sylar