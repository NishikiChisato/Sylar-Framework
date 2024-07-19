#include "../src/include/coroutine.hh"
#include "../src/include/hook.hh"
#include "../src/include/socket.hh"

void HandleClientRead(Sylar::Socket::ptr clientfd) {
  auto &buf = clientfd->GetBuffer();
  buf.resize(1024);
  ssize_t read_bytes = 0;
  while (true) {
    std::cout << "HandleClientRead: " << clientfd->GetSocket() << std::endl;
    auto &buf = clientfd->GetBuffer();
    if (clientfd->Recv(buf.data(), buf.size()) > 0) {
      std::cout << "server receive: " << buf.data() << std::endl;
    }
    Sylar::Schedule::Yield();
  }
}

void HandleClientWrite(Sylar::Socket::ptr clientfd) {
  while (true) {
    std::cout << "HandleClientWrite: " << clientfd->GetSocket() << std::endl;
    auto &buf = clientfd->GetBuffer();
    if (!buf.empty()) {
      clientfd->Send(buf.data(), clientfd->GetBufferSize());
    }
    Sylar::Schedule::Yield();
  }
}

void AcceptCo(Sylar::Socket::ptr listenfd) {
  while (true) {
    auto clientfd = listenfd->Accept();

    if (clientfd) {
      std::cout << "new client connect: " << clientfd->GetAddress()->ToString()
                << ", socket fd: " << clientfd->GetSocket() << std::endl;
      auto epoll = Sylar::Schedule::GetThreadEpoll();

      auto attr = std::make_shared<Sylar::CoroutineAttr>();
      auto r_co = Sylar::Coroutine::CreateCoroutine(
          Sylar::Schedule::GetThreadSchedule(), attr,
          std::bind(&HandleClientRead, clientfd));

      auto w_co = Sylar::Coroutine::CreateCoroutine(
          Sylar::Schedule::GetThreadSchedule(), attr,
          std::bind(&HandleClientWrite, clientfd));

      epoll->RegisterEvent(Sylar::Epoll::EventType::READ |
                               Sylar::Epoll::EventType::WRITE,
                           clientfd->GetSocket(), nullptr, nullptr, r_co, w_co);
    }

    Sylar::Schedule::Yield();
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "usage: " << argv[0] << " + ip address + port number"
              << std::endl;
    return 0;
  }
  std::string host;
  host += std::string(argv[1]);
  host += ":";
  host += std::string(argv[2]);
  std::cout << host << std::endl;
  auto addr = Sylar::Address::LookupAnyIPAddress(host);

  auto listenfd = Sylar::Socket::CreateTCPIPv4Socket();
  listenfd->Bind(addr);
  listenfd->Listen();

  auto attr = std::make_shared<Sylar::CoroutineAttr>();
  auto accept_co =
      Sylar::Coroutine::CreateCoroutine(Sylar::Schedule::GetThreadSchedule(),
                                        attr, std::bind(&AcceptCo, listenfd));

  auto epoll = Sylar::Schedule::GetThreadEpoll();

  epoll->RegisterEvent(Sylar::Epoll::EventType::READ |
                           Sylar::Epoll::EventType::WRITE,
                       listenfd->GetSocket(), nullptr, nullptr, accept_co);

  Sylar::Schedule::Eventloop(epoll);
}