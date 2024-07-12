#ifndef __SYLAR_FDCONTEXT_H__
#define __SYLAR_FDCONTEXT_H__

#include "./log.h"
#include "./mutex.h"
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace Sylar {

class FDContext {
public:
  typedef std::shared_ptr<FDContext> ptr;
  FDContext(int fd);
  ~FDContext() {}

  bool IsInit() { return is_init_; }

  bool IsClose() { return is_close_; }

  bool IsSocket() { return is_socket_; }

  bool IsFifo() { return is_fifo_; }

  bool IsNonBlock() { return is_nonblock_; }

  void SetNonBlock(bool v);

  /**
   * @param type SO_RCVTIMEO(recv timeout), SO_SNDTIME)(send timeout)
   */
  void SetTimeout(int type, uint64_t val);

  uint64_t GetTimeout(int type);

private:
  void Init();

private:
  int fd_;         // file descriptor
  bool is_init_;   // init or not
  bool is_socket_; // socket or not
  bool is_close_;  // close or not
  bool is_fifo_;
  bool is_nonblock_; // non-block or not

  uint64_t recv_timeout_;
  uint64_t send_timeout_;
};

class FDManager {
public:
  FDContext::ptr GetFD(int fd, bool auto_create = true);

  void DeleteFD(int fd);

  static FDManager &Instance() {
    static FDManager instance;
    return instance;
  }

private:
  FDManager();
  ~FDManager() {}

  Mutex mu_;
  std::vector<FDContext::ptr> fds_;
};

} // namespace Sylar

#endif