#include "../src/include/socket.h"
#include <gtest/gtest.h>

TEST(Socket, Basic) {
  std::string host = "www.bilibili.com:http";
  Sylar::IPAddress::ptr addr = Sylar::IPAddress::LookupAnyIPAddress(host);
  // addr->SetPort(80);
  if (addr) {
    std::cout << host << " ip: " << addr->ToString() << std::endl;
  } else {
    std::cerr << "LookupAnyIPAddress error" << std::endl;
  }
  Sylar::Socket::ptr sock = Sylar::Socket::CreateTCPIPv4Socket();

  sock->Connect(addr);
  std::string buf = "GET / HTTP/1.1\r\n\r\n";
  std::cout << "data to be send..." << std::endl;
  sock->Send(buf.c_str(), buf.length());

  std::string rcv;
  rcv.resize(4096);
  std::cout << "try to receive data..." << std::endl;
  sock->Recv(&rcv[0], rcv.length());
  std::cout << rcv << std::endl;
}