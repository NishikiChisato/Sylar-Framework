#include "../src/include/hook.h"
#include "../src/include/socket.h"
#include <gtest/gtest.h>

TEST(Socket, DISABLED_Client) {
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

TEST(Socket, Server) {
  std::string host = "127.0.0.1:15952";
  auto addr = Sylar::IPAddress::LookupAnyIPAddress(host);
  if (addr) {
    std::cout << host << " ip: " << addr->ToString() << std::endl;
  } else {
    std::cerr << "LookupAnyIPAddress error" << std::endl;
  }
  Sylar::Socket::ptr sock = Sylar::Socket::CreateTCPIPv4Socket();

  sock->Bind(addr);
  std::cout << "socket bind success" << std::endl;
  sock->Listen();
  std::cout << "socket listen success" << std::endl;

  while (true) {
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    auto client = sock->Accept();
    if (client) {
      std::cout << "client connect success" << std::endl;
    } else {
      std::cerr << "client connect error: " << strerror(errno) << std::endl;
    }
    client->Recv(buf, 1024);
    std::cout << "receive from client: " << client->ToString();
    std::cout << "content: " << buf << std::endl;
    std::cout << "write identical data to client" << std::endl;
    client->Send(buf, sizeof(buf));

    client->Close();
  }
}