#include "../src/include/tcp_setver.hh"
#include <exception>
#include <gtest/gtest.h>

class EchoServer : public Sylar::TcpServer {
public:
  EchoServer(const std::string &name) : TcpServer(name) {}

  void HandleClient(Sylar::Socket::ptr socket);
};

void EchoServer::HandleClient(Sylar::Socket::ptr socket) {
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "handling client: " << socket->ToString();
  while (!IsStop()) {
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    int ret = socket->Recv(buf, 1024);
    if (ret == 0) {
      SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "client close";
      else {
        SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "client send data is: " << buf;
      }
    }
  }
}

void Tsk1() {
  EchoServer server("echo server");
  auto addr = Sylar::Address::LookupAnyIPAddress("127.0.0.1:15998");
  server.Bind(addr);
  server.Start();
}

TEST(TcpServer, EchoServer) {
  Sylar::IOManager::ptr iom(new Sylar::IOManager(1, true));
  iom->Start();
  iom->ScheduleTask(&Tsk1);
  iom->Stop();
}