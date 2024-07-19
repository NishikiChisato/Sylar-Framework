#include "../src/include/address.hh"
#include <gtest/gtest.h>

TEST(Address, Basic) {
  auto ipv4 = Sylar::IPv4Address::Create("127.0.0.1", 80);
  EXPECT_EQ(ipv4->ToString(), "127.0.0.1:80");
  auto ipv6 = Sylar::IPv6Address::Create("::1", 80);
  EXPECT_EQ(ipv6->ToString(), "[::1]:80");

  std::vector<Sylar::Address::ptr> vaddr;
  if (Sylar::Address::Lookup(vaddr, "www.google.com:http", AF_INET, SOCK_STREAM,
                             0)) {
    for (auto &item : vaddr) {
      std::cout << "IPv4: " << item->ToString() << std::endl;
    }
  }

  if (Sylar::Address::Lookup(vaddr, "www.google.com:http", AF_INET6,
                             SOCK_STREAM, 0)) {
    for (auto &item : vaddr) {
      std::cout << "IPv6: " << item->ToString() << std::endl;
    }
  }
}