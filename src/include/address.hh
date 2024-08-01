#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include "log.hh"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

namespace Sylar {

class IPAddress;
class IPv4Address;
class IPv6Address;

// abstract base class for address
class Address {
public:
  typedef std::shared_ptr<Address> ptr;

  /**
   * @brief create different address based on different type of sockaddr
   *
   * @param [in] addr the pointer of sockaddr
   * @param [in] addrlen the lenght of sockaddr
   * @return return the Address object which matchs the type of sockaddr
   */
  static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);

  /**
   * @brief return all address by using hostname
   *
   * @param [out] result the corresponding address of this hostname
   * @param [in] host hostname: www.google.com:http / 127.0.0.1:http /
   * [::1]:http
   * @param [in] family protocol family: AF_INET or AF_INET6
   * @param [in] type the type of socket: SOCK_STREAM or SOCK_DGRAM
   * @param [in] protocol the protocol of socket: IPPROTO_TCP or IPPROTO_UDP
   * @return true if success; otherwise return false
   */
  static bool Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family, int type, int protocol);

  /**
   * @brief return any address by using hostname
   *
   * @param [out] result the corresponding address of this hostname
   * @param [in] host hostname: www.google.com:http / 127.0.0.1:http /
   * [::1]:http
   * @param [in] family protocol family: AF_INET or AF_INET6
   * @param [in] type the type of socket: SOCK_STREAM or SOCK_DGRAM
   * @param [in] protocol the protocol of socket: IPPROTO_TCP or IPPROTO_UDP
   * @return Address pointer if success; otherwise return nullptr
   */
  static Address::ptr LookupAny(const std::string &host, int family, int type,
                                int protocol);

  /**
   * @brief return any IP address by using hostname
   *
   * @param [out] result the corresponding address of this hostname
   * @param [in] host hostname: www.google.com:http / 127.0.0.1:http /
   * [::1]:http
   * @param [in] family protocol family: AF_INET or AF_INET6
   * @param [in] type the type of socket: SOCK_STREAM or SOCK_DGRAM
   * @param [in] protocol the protocol of socket: IPPROTO_TCP or IPPROTO_UDP
   * @return Address pointer if success; otherwise return nullptr
   */
  static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string &host,
                                                       int family = AF_INET,
                                                       int type = SOCK_STREAM,
                                                       int protocol = 0);

  virtual ~Address() {}

  /**
   * @brief get protocol family of current address
   *
   * @return int AF_INET or AF_INET6
   */
  virtual int GetFamily() const = 0;

  /**
   * @brief return sockaddr of current address
   *
   * @return sockaddr*
   */
  virtual sockaddr *GetSockaddr() = 0;

  /**
   * @brief return the length of current address
   *
   * @return socklen_t
   */
  virtual socklen_t GetSocklen() const = 0;

  /**
   * @brief used to output address
   *
   * @return std::ostream&
   */
  virtual std::ostream &Insert(std::ostream &os) = 0;

  std::string ToString();
};

/**
 * abstract base class of ip address
 */
class IPAddress : public Address {
public:
  typedef std::shared_ptr<IPAddress> ptr;

  /**
   * @brief create an different type of IP address
   *
   * @param address a hostname or dotted-decimal format of ip address
   * @param port port number
   * @return return pointer of IPv4 or IPv6 if success; otherwise return false
   */
  static IPAddress::ptr Create(const std::string &address, uint32_t port);

  virtual uint32_t GetPort() const = 0;

  virtual void SetPort(uint32_t val) = 0;
};

class IPv4Address : public IPAddress {
public:
  typedef std::shared_ptr<IPv4Address> ptr;

  /**
   * @brief Construct a new IPv4Address object by using sockaddr_in
   *
   * @param address
   */
  IPv4Address(const sockaddr_in &address);

  /**
   * @brief Construct a new IPv4Address object by using dotted-decimal address
   * and port number
   *
   * @param [in] address dotted-decimal number address
   * @param [in] port port number
   */
  IPv4Address(const std::string &address = "127.0.0.1", uint16_t port = 80);

  /**
   * @brief create an ipv4 address by using dotted-decimal number
   *
   * @param [in] address dotted-decimal number
   * @param [in] port port number whit host byte order
   * @return IPv4Address::ptr
   */
  static IPv4Address::ptr Create(const std::string &address, uint16_t port);

  /**
   * @brief return the family field of sockaddr_in
   */
  int GetFamily() const override { return addr_in_.sin_family; }

  sockaddr *GetSockaddr() override { return (sockaddr *)&addr_in_; }

  socklen_t GetSocklen() const override { return sizeof(addr_in_); }

  std::ostream &Insert(std::ostream &os) override;

  /**
   * @brief return the port number with host byth order
   */
  uint32_t GetPort() const override { return ntohs(addr_in_.sin_port); }

  /**
   * @brief Set the Port object
   *
   * @param [in] val new port number with host byte order
   */
  void SetPort(uint32_t val) override { addr_in_.sin_port = htons(val); }

private:
  sockaddr_in addr_in_;
};

class IPv6Address : public IPAddress {
public:
  typedef std::shared_ptr<IPv6Address> ptr;

  IPv6Address();

  IPv6Address(const sockaddr_in6 &address);

  IPv6Address(const std::string &address, uint16_t port);

  /**
   * @brief create an ipv6 address
   *
   * @param address dotted-decimal number
   * @param port port number
   * @return IPv4Address::ptr
   */
  static IPv6Address::ptr Create(const std::string &address, uint16_t port);

  int GetFamily() const override { return addr_in6_.sin6_family; }

  sockaddr *GetSockaddr() override { return (sockaddr *)&addr_in6_; }

  socklen_t GetSocklen() const override { return sizeof(addr_in6_); }

  std::ostream &Insert(std::ostream &os) override;

  uint32_t GetPort() const override { return ntohs(addr_in6_.sin6_port); }

  void SetPort(uint32_t val) override { addr_in6_.sin6_port = ntohs(val); }

private:
  sockaddr_in6 addr_in6_;
};

} // namespace Sylar

#endif