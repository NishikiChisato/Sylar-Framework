#include "include/address.h"

namespace Sylar {

template <typename T> static T CreateMask(uint32_t val) {
  return ~((1 << (sizeof(T) - val)) - 1);
}

template <typename T> static uint32_t BitCount(T val) {
  uint32_t res = 0;
  while (val) {
    // delete the rightest zeor
    val &= val - 1;
    res++;
  }
  return res;
}

Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
  if (addr == nullptr) {
    return nullptr;
  }
  switch (addr->sa_family) {
  case AF_INET:
    return IPv4Address::ptr(new IPv4Address(*(const sockaddr_in *)addr));
  case AF_INET6:
    return IPv6Address::ptr(new IPv6Address(*(const sockaddr_in6 *)addr));
  default:
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Address::Create protocol family not support: " << addr->sa_family;
    throw std::runtime_error("Address::Create protocol family not support");
  }
}

bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family, int type, int protocol) {
  std::string node;
  std::string service;
  addrinfo hint, *res, *nxt;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = family;
  hint.ai_socktype = type;
  hint.ai_protocol = protocol;

  /**
   * we first check ipv6 address, the format of ipv6 is as follows:
   */
  if (!host.empty() && host[0] == '[') {
    char *endaddr = (char *)memchr(host.c_str() + 1, ']', host.size() - 1);
    if (endaddr) {
      endaddr++;
      if (*endaddr == ':') {
        service.assign(endaddr + 1, host.size() - (endaddr + 1 - host.c_str()));
        node = host.substr(0, endaddr - host.c_str());
      }
    }
  }

  /**
   * host is not ipv6 address
   */
  if (node.empty()) {
    char *endaddr = (char *)memchr(host.c_str(), ':', host.size());
    if (endaddr) {
      service.assign(endaddr + 1, host.size() - (endaddr + 1 - host.c_str()));
      node = host.substr(0, endaddr - host.c_str());
    }
  }

  /**
   * host neither ipv4 nor ipv6, we directly treat it as hostname
   */
  if (node.empty()) {
    node = host;
  }

  int err = getaddrinfo(node.c_str(), service.c_str(), &hint, &res);
  if (err) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "Address::Lookup error: " << strerror(errno);
    throw std::runtime_error(strerror(errno));
  }
  nxt = res;
  while (nxt) {
    result.push_back(Create(nxt->ai_addr, nxt->ai_addrlen));
    nxt = nxt->ai_next;
  }
  freeaddrinfo(res);
  return !result.empty();
}

Address::ptr Address::LookupAny(const std::string &host, int family, int type,
                                int protocol) {
  std::vector<Address::ptr> res;
  if (Address::Lookup(res, host, family, type, protocol)) {
    return res[0];
  }
  return nullptr;
}

std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string &host,
                                                       int family, int type,
                                                       int protocol) {
  std::vector<Address::ptr> res;
  if (Address::Lookup(res, host, family, type, protocol)) {
    for (auto &item : res) {
      auto p = std::dynamic_pointer_cast<IPAddress>(item);
      return p;
    }
  }
  return nullptr;
}

std::string Address::ToString() {
  std::stringstream ss;
  Insert(ss);
  return ss.str();
}

IPAddress::ptr IPAddress::Create(const std::string &address, uint32_t port) {
  addrinfo hint, *res;
  hint.ai_family = AF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;
  int err = getaddrinfo(address.c_str(), nullptr, &hint, &res);
  if (err) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "IPAddress::Create error: " << strerror(errno);
    throw std::runtime_error(strerror(errno));
  }
  IPAddress::ptr ptr = std::dynamic_pointer_cast<IPAddress>(
      Address::Create(res->ai_addr, res->ai_addrlen));
  if (ptr) {
    ptr->SetPort(port);
  }
  freeaddrinfo(res);
  return ptr;
}

IPv4Address::IPv4Address(const sockaddr_in &address) { addr_in_ = address; }

IPv4Address::IPv4Address(const std::string &address, uint16_t port) {
  addr_in_.sin_family = AF_INET;
  addr_in_.sin_port = htons(port);
  int err = inet_pton(AF_INET, address.c_str(), &addr_in_.sin_addr);
  if (err < 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "IPv4::Constructor error: " << strerror(errno);
    throw std::runtime_error(strerror(errno));
  } else if (err == 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "IPv4::Constructor error: network address invalid";
  }
}

IPv4Address::ptr IPv4Address::Create(const std::string &address,
                                     uint16_t port) {
  IPv4Address::ptr ptr(new IPv4Address());
  ptr->addr_in_.sin_family = AF_INET;
  ptr->addr_in_.sin_port = htons(port);
  int err = inet_pton(AF_INET, address.c_str(), &ptr->addr_in_.sin_addr);
  if (err < 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "IPv4::Create error: " << strerror(errno);
    throw std::runtime_error(strerror(errno));
  } else if (err == 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "IPv4::Create error: network address invalid";
  }
  return ptr;
}

std::ostream &IPv4Address::Insert(std::ostream &os) {
  char buf[32];
  memset(buf, 0, sizeof(buf));
  inet_ntop(AF_INET, &addr_in_.sin_addr, (char *)buf, INET_ADDRSTRLEN);
  // std::cout << ((buf >> 24) & 0xff) << "." << ((buf >> 16) & 0xff) << "."
  //           << ((buf >> 8) & 0xff) << "." << (buf & 0xff) << ":";
  os << buf << ":";
  os << GetPort();
  return os;
}

IPv6Address::IPv6Address() {
  memset(&addr_in6_, 0, sizeof(addr_in6_));
  addr_in6_.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6 &address) { addr_in6_ = address; }

IPv6Address::IPv6Address(const std::string &address, uint16_t port) {
  addr_in6_.sin6_port = htons(port);
  int err = inet_pton(AF_INET6, address.c_str(), &addr_in6_.sin6_addr);
  if (err < 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "IPv6::Constructor error: " << strerror(errno);
    throw std::runtime_error(strerror(errno));
  } else if (err == 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "IPv6::Constructor error: network address invalid";
  }
}

IPv6Address::ptr IPv6Address::Create(const std::string &address,
                                     uint16_t port) {
  IPv6Address::ptr ptr(new IPv6Address());
  ptr->addr_in6_.sin6_port = htons(port);
  int err = inet_pton(AF_INET6, address.c_str(), &ptr->addr_in6_.sin6_addr);
  if (err < 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "IPv6::Create error: " << strerror(errno);
    throw std::runtime_error(strerror(errno));
  } else if (err == 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "IPv6::Create error: network address invalid";
  }
  return ptr;
}

std::ostream &IPv6Address::Insert(std::ostream &os) {
  char buf[128];
  memset(buf, 0, sizeof(buf));
  inet_ntop(AF_INET6, &addr_in6_.sin6_addr, (char *)&buf, INET6_ADDRSTRLEN);
  os << "[" << buf << "]:" << GetPort();
  return os;
}

} // namespace Sylar
