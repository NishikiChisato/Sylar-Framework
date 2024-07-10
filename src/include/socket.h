#ifndef __SYLAR_SOCKETWRAP_H__
#define __SYLAR_SOCKETWRAP_H__

#include "./address.h"
#include "./fdcontext.h"
#include "./hook.h"
#include <boost/noncopyable.hpp>
#include <memory>
#include <netinet/tcp.h>
#include <sys/types.h>

namespace Sylar {

class Socket : public std::enable_shared_from_this<Socket>,
               public boost::noncopyable {
public:
  typedef std::shared_ptr<Socket> ptr;

  enum SocketType {
    TCP = SOCK_STREAM, // TCP socket
    UDP = SOCK_DGRAM   // UDP socket
  };

  enum Family {
    IPv4 = AF_INET, // IPv4 socket
    IPv6 = AF_INET6 // IPv6 socket
  };

  static Socket::ptr CreateTCPSocket(const Address::ptr addr);

  static Socket::ptr CreateUDPSocket(const Address::ptr addr);

  static Socket::ptr CreateTCPIPv4Socket() {
    return Socket::ptr(new Socket(IPv4, TCP, 0));
  }

  static Socket::ptr CreateTCPIPv6Socket() {
    return Socket::ptr(new Socket(IPv6, TCP, 0));
  }

  static Socket::ptr CreateUDPIPv4Socket() {
    return Socket::ptr(new Socket(IPv4, UDP, 0));
  }

  static Socket::ptr CreateUDPIPv6Socket() {
    return Socket::ptr(new Socket(IPv6, UDP, 0));
  }

  Socket(int family, int type, int protocol = 0);

  virtual ~Socket();

  /**
   * @see getsockopt
   */
  bool GetSockOpt(int level, int option, void *result, socklen_t *len);

  /**
   * @see setsockopt
   */
  bool SetSockOpt(int level, int option, void *result, socklen_t len);

  /**
   * @brief accept connection
   *
   * @attention socket should bind and listen first
   *
   * @return return new created connected socket if success; otherwise return
   * nullptr
   */
  virtual Socket::ptr Accept();

  /**
   * @brief bind current socket with specified address
   *
   * @param addr local address
   */
  virtual bool Bind(const Address::ptr addr);

  /**
   * @brief connect to destination address
   *
   * @param addr remote address
   */
  virtual bool Connect(const Address::ptr addr);

  /**
   * @brief listen current socket
   *
   * @param backlog the max size of queue of unconnected sock
   *
   * @return true if success; otherwise return false
   */
  virtual bool Listen(int backlog = SOMAXCONN);

  /**
   * @brief close this socket
   *
   * @return true if success; otherwise return false
   */
  virtual bool Close();

  virtual ssize_t Send(const void *buf, size_t len, int flags = 0);

  virtual ssize_t SendTo(const void *buf, size_t len, int flags,
                         sockaddr *dest_addr, socklen_t addrlen);

  virtual ssize_t SendMsg(const msghdr *msg, int flags);

  virtual ssize_t Recv(void *buf, size_t len, int flags = 0);

  virtual ssize_t RecvFrom(void *buf, size_t len, int flags, sockaddr *src_addr,
                           socklen_t *addrlen);

  virtual ssize_t RecvMsg(msghdr *msg, int flags);

  Address::ptr GetRemoteAddress();

  Address::ptr GetLocalAddress();

  int GetSocket() const { return socket_; }

  int GetFamily() const { return family_; }

  int GetType() const { return type_; }

  int GetProtocol() const { return protocol_; }

  bool IsConnected() const { return is_connected_; }

  virtual std::string ToString();

  /**
   * @brief cancel read event
   */
  bool CancelReadEvent() {
    return IOManager::GetThis()->CancelEvent(socket_, IOManager::READ);
  }

  /**
   * @brief cancel write event
   */
  bool CancelWriteEvent() {
    return IOManager::GetThis()->CancelEvent(socket_, IOManager::WRITE);
  }

  /**
   * @brief cancel all event
   */
  bool CancelAllEvent() {
    return IOManager::GetThis()->CancelAllEvent(socket_);
  }

protected:
  /**
   * @brief init identical information of UDP socket or TCP socket
   */
  void InitSocket();

  /**
   * @brief use socket file descriptor to init current socket
   *
   * @param socket file descriptor
   */
  void Init(int socket);

protected:
  int socket_;        // socket file descriptor
  int family_;        // the family of socket
  int type_;          // the type of socket
  int protocol_;      // the protocol arguement of socket()
  bool is_connected_; // connected or not(UDP is connectless, so this field is
                      // always true)
  Address::ptr remote_address_; // the remote end of socket
  Address::ptr local_address_;  // the local end of socket
};

} // namespace Sylar

#endif