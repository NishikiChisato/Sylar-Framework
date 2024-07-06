#ifndef __SYLAR_FDCONTEXT_H__
#define __SYLAR_FDCONTEXT_H__

#include "./log.h"
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace Sylar {

class FDContext {
public:
  FDContext(int fd);
  ~FDContext() {}

  bool IsInit() { return is_init_; }

  bool IsClose() { return is_close_; }

  bool IsSocket() { return is_socket_; }

  bool IsNonBlock() { return is_nonblock_; }

  void SetNonBlock(bool v);

  /**
   * @param type SO_RCVTIMEO -> read timeout; SO_SNDTIMEO -> write timeout
   * @return return timeout if type is SO_RCVTIMEO or SO_SNDTIMEO; return ~0ull
   * if not
   */
  uint64_t GetTimeout(int type);

  void SetTimeout(int type, uint64_t timeout);

private:
  void Init();

private:
  int fd_;           // file descriptor
  bool is_init_;     // init or not
  bool is_socket_;   // socket or not
  bool is_close_;    // close or not
  bool is_nonblock_; // non-block or not

  uint64_t read_timeout_;  // read event timeout
  uint64_t write_timeout_; // write event timeout
};

class FDManager {
public:
private:
};

} // namespace Sylar

#endif