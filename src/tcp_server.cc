#include "include/tcp_setver.hh"

namespace Sylar {

TcpServer::TcpServer(const std::string &name) : name_(name) {
  iomgr_ = IOManager::GetThis();
  acptmgr_ = IOManager::GetThis();
  recv_timeout_ = 1000;
  is_stoped_ = true;
}

bool TcpServer::Bind(Address::ptr addr) {
  // use address to create different type of tcp socket, this stop only create
  // different type of socket, not bind socket
  auto sock = Socket::CreateTCPSocket(addr);
  if (!sock->Bind(addr)) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "TcpServer::Bind bind socket failed: " << strerror(errno);
    return false;
  }
  if (!sock->Listen()) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "TcpServer::Bind listen socket failed: " << strerror(errno);
    return false;
  }
  listened_socket_.push_back(sock);
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT)
      << "server name: " << name_ << "listen socket: " << sock->ToString();
  return true;
}

bool TcpServer::Start() {
  if (!is_stoped_) {
    return true;
  }
  is_stoped_ = false;
  for (auto &sock : listened_socket_) {
    acptmgr_->ScheduleTask(std::bind(&TcpServer::AcceptClient, this, sock));
  }
  return true;
}

/**
 * @brief this method just try to close socket
 *
 */
void TcpServer::Stop() {
  is_stoped_ = true;
  for (auto &sock : listened_socket_) {
    sock->CancelAllEvent();
    sock->Close();
  }
}

void TcpServer::AcceptClient(Socket::ptr socket) {
  while (!IsStop()) {
    std::cout << "TcpServer::AcceptClient enter" << std::endl;
    std::cout.flush();
    auto client = socket->Accept();
    if (client) {
      iomgr_->ScheduleTask(std::bind(&TcpServer::HandleClient, this, client));
    } else {
      SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
          << "TcpServer::AcceptClient socket accept failed: "
          << strerror(errno);
    }
  }
}

} // namespace Sylar