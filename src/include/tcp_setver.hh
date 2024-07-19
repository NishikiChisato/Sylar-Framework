#ifndef __SYLAR_TCPSERVER_H__
#define __SYLAR_TCPSERVER_H__

#include "address.hh"
#include "bytearray.hh"
#include "coroutine.hh"
#include "socket.hh"

namespace Sylar {

class TcpServer : std::enable_shared_from_this<TcpServer> {
public:
  TcpServer(const std::string &name);

  virtual ~TcpServer() {}

  /**
   * @brief tcp server should has one or more listen address. this function used
   * to create those listen socket
   *
   * @param addr address of listen socket
   */
  virtual bool Bind(Sylar::Address::ptr addr);

  /**
   * @pre pre-call Bind()
   */
  virtual bool Start();

  virtual void Stop();

  bool IsStop() { return is_stoped_; }

  void SetRecvTimeout(uint64_t v) { recv_timeout_ = v; }

  uint64_t GetRecvTimeout() { return recv_timeout_; }

protected:
  /**
   * @brief derived class should overwrite this function
   *
   * @param socket represent client socket
   */
  virtual void HandleClient(Socket::ptr socket) = 0;

  virtual void AcceptClient(Socket::ptr socket);

private:
  std::vector<Socket::ptr>
      listened_socket_; // this socket serve as accpet socket
  std::string name_;
  std::shared_ptr<Coroutine> ac_co_;
  uint64_t recv_timeout_; // recv timeout(ms)
  bool is_stoped_;
};

} // namespace Sylar

#endif