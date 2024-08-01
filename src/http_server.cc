#include "include/http_server.hh"

namespace Sylar {

namespace http {

HttpServer::HttpServer(const std::string &host, std::uint16_t port, bool use_co)
    : host_(host), port_(port), rdev_(), random_engine_(rdev_()),
      sleep_time_(10, 100), use_co_(use_co) {
  // create socket
  sock_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (sock_fd_ < 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "[HttpServer::HttpServer] socket failed: " << strerror(errno);
    throw std::runtime_error(
        "[HttpServer::HttpServer] failed to create socket");
  }
  // resize for std::vector
  worker_threads_.resize(kThreadPoolSize);
  worker_epoll_fd_.resize(kThreadPoolSize);
  worker_epoll_event_.resize(kThreadPoolSize);
  for (int i = 0; i < kThreadPoolSize; i++) {
    worker_epoll_event_[i].resize(kMaxEvent);
  }
}

void HttpServer::Start() {
  // setup socket
  int opt = 1;
  if (setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) < 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "[HttpServer::HttpServer] setsocketopt failed: " << strerror(errno);
    throw std::runtime_error("[HttpServer::HttpServer] set socketopt failed");
  }
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_);
  inet_pton(AF_INET, host_.c_str(), &addr.sin_addr.s_addr);

  if (bind(sock_fd_, (sockaddr *)&addr, sizeof(addr)) < 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "[HttpServer::HttpServer] bind failed: " << strerror(errno);
    throw std::runtime_error("[HttpServer::HttpServer] socket bind failed");
  }

  if (listen(sock_fd_, SOMAXCONN) < 0) {
    SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
        << "[HttpServer::HttpServer] listen failed: " << strerror(errno);
    throw std::runtime_error("[HttpServer::HttpServer] socket listen failed");
  }

  // setup epoll
  for (int i = 0; i < kThreadPoolSize; i++) {
    if ((worker_epoll_fd_[i] = epoll_create1(0)) < 0) {
      SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
          << "[HttpServer::HttpServer] epoll_create1 failed: "
          << strerror(errno);
      throw std::runtime_error("[HttpServer::HttpServer] epoll create failed");
    }
  }

  // running thread
  running_ = true;
  listener_thread_ = std::thread(&HttpServer::ListenClient, this);
  for (int i = 0; i < kThreadPoolSize; i++) {
    if (use_co_) {
      worker_threads_[i] = std::thread(&HttpServer::EventLoopWithCo, this, i);
    } else {
      worker_threads_[i] = std::thread(&HttpServer::EventLoop, this, i);
    }
  }
}

void HttpServer::Stop() {
  listener_thread_.join();
  for (int i = 0; i < kThreadPoolSize; i++) {
    worker_threads_[i].join();
  }
  for (int i = 0; i < kThreadPoolSize; i++) {
    close(worker_epoll_fd_[i]);
  }
  close(sock_fd_);
}

void HttpServer::ListenClient() {
  EventData *client_data = nullptr;
  sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  bool active = true;
  int worker_idx = 0;
  while (running_) {
    if (!active) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(sleep_time_(random_engine_)));
    }
    int client_fd =
        accept4(sock_fd_, (sockaddr *)&client_addr, &client_len, SOCK_NONBLOCK);
    if (client_fd < 0) {
      active = false;
      continue;
    }
    std::cout << "client connect, fd is " << client_fd << std::endl;
    active = true;
    // register in epoll
    client_data = new EventData();
    client_data->fd_ = client_fd;
    ControlEpollEvent(worker_epoll_fd_[worker_idx++], EPOLL_CTL_ADD, client_fd,
                      EPOLLIN | EPOLLET, client_data);
    worker_idx %= kThreadPoolSize;
  }
}

void HttpServer::EventLoop(int thread) {
  EventData *client_data = nullptr;
  int epfd = worker_epoll_fd_[thread];
  auto &events = worker_epoll_event_[thread];
  bool active = true;
  while (running_) {
    if (!active) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(sleep_time_(random_engine_)));
    }
    int cnt = epoll_wait(epfd, events.data(), kMaxEvent, kMaxEpollWaitTimeout);
    if (cnt <= 0) {
      active = false;
      continue;
    }
    active = true;
    for (int i = 0; i < cnt; i++) {
      auto &ev = events[i];
      client_data = reinterpret_cast<EventData *>(ev.data.ptr);
      if ((ev.events & EPOLLHUP) || (ev.events & EPOLLERR)) {
        // the peer close socket
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_data->fd_);
        close(client_data->fd_);
        delete client_data;
      } else if ((ev.events & EPOLLIN) || (ev.events & EPOLLOUT)) {
        HandleEpollEvent(epfd, ev.events, client_data);
      } else {
        // unexpective error occur
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_data->fd_);
        close(client_data->fd_);
        delete client_data;
      }
    }
  }
}

void HttpServer::HandleEpollEvent(int epfd, std::uint32_t event,
                                  EventData *data) {
  int client_fd = data->fd_;
  EventData *request = nullptr;
  EventData *response = nullptr;
  if (event & EPOLLIN) {
    request = data;
    auto bytes = recv(client_fd, request->buf_, kMaxBufferSize, 0);
    if (bytes > 0) {
      request->length_ = bytes;
      response = new EventData();
      response->fd_ = client_fd;
      HandleRequestData(request, response);
      // if we have successfully generating http response message, which is
      // stored in EventData. we store this EventData in pointer of epoll_wait
      // and register WRITE event
      ControlEpollEvent(epfd, EPOLL_CTL_MOD, client_fd, EPOLLOUT | EPOLLET,
                        response);
    } else if (bytes == 0) {
      // client close connection
      ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_fd);
      close(client_fd);
      delete request;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        ControlEpollEvent(epfd, EPOLL_CTL_MOD, client_fd, EPOLLIN | EPOLLET,
                          request);
      } else {
        // unexpective error occur
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_fd);
        close(client_fd);
        delete request;
      }
    }
  } else {
    response = data;
    auto bytes = send(client_fd, response->buf_ + response->cursor_,
                      response->length_, 0);
    if (bytes > 0) {
      // we write part of response message to client, we should write again
      response->cursor_ += bytes;
      response->length_ -= bytes;
      ControlEpollEvent(epfd, EPOLL_CTL_MOD, client_fd, EPOLLOUT | EPOLLET,
                        response);
    } else if (bytes == 0) {
      // we have written all response message to client, we register READ for
      // client fd and wait client request message again
      if (response->keep_alive_) {
        delete response;
        request = new EventData();
        request->fd_ = client_fd;
        ControlEpollEvent(epfd, EPOLL_CTL_MOD, client_fd, EPOLLIN | EPOLLET,
                          request);
      } else {
        delete response;
        // don't keep alive
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_fd);
        close(client_fd);
      }
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        ControlEpollEvent(epfd, EPOLL_CTL_MOD, client_fd, EPOLLOUT | EPOLLET,
                          response);
      } else {
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_fd);
        close(client_fd);
        delete response;
      }
    }
  }
}

void HttpServer::EventLoopWithCo(int thread) {
  auto schedule = Schedule::GetThreadSchedule();
  int epfd = worker_epoll_fd_[thread];
  auto &events = worker_epoll_event_[thread];
  EventData *client_data = nullptr;
  while (running_) {
    int cnt = epoll_wait(epfd, events.data(), kMaxEvent, kMaxEpollWaitTimeout);
    if (cnt <= 0) {
      continue;
    }
    for (int i = 0; i < cnt; i++) {
      auto &ev = events[i];
      client_data = reinterpret_cast<EventData *>(ev.data.ptr);
      if ((ev.events & EPOLLHUP) || (ev.events & EPOLLERR)) {
        // the peer close socket
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_data->fd_);
        close(client_data->fd_);
        delete client_data;
      } else if (ev.events & EPOLLIN) {
        auto attr = std::make_shared<CoroutineAttr>();
        auto rco = Coroutine::CreateCoroutine(
            schedule, attr,
            std::bind(&HttpServer::HandleReadEvent, this, epfd, client_data));
        rco->Resume();
      } else if (ev.events & EPOLLOUT) {
        auto attr = std::make_shared<CoroutineAttr>();
        auto wco = Coroutine::CreateCoroutine(
            schedule, attr,
            std::bind(&HttpServer::HandleWriteEvent, this, epfd, client_data));
        wco->Resume();
      } else {
        // unexpective error
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_data->fd_);
        close(client_data->fd_);
        delete client_data;
      }
    }
  }
}

void HttpServer::HandleReadEvent(int epfd, EventData *data) {
  EventData *request = data;
  EventData *response = nullptr;
  int client_fd = request->fd_;
  while (true) {
    if (!request) {
      Schedule::Yield();
    }
    auto bytes = recv(client_fd, request->buf_, kMaxBufferSize, 0);
    if (bytes > 0) {
      request->length_ = bytes;
      response = new EventData();
      response->fd_ = client_fd;
      HandleRequestData(request, response);
      ControlEpollEvent(epfd, EPOLL_CTL_MOD, client_fd, EPOLLOUT | EPOLLET,
                        response);
      Schedule::Yield();
    } else if (bytes == 0) {
      ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_fd);
      close(client_fd);
      delete request;
      request = nullptr;
      Schedule::Yield();
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        Schedule::Yield();
      } else {
        // unexpective error occur
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_fd);
        close(client_fd);
        delete request;
        request = nullptr;
        Schedule::Yield();
      }
    }
  }
}

void HttpServer::HandleWriteEvent(int epfd, EventData *data) {
  EventData *request = nullptr;
  EventData *response = data;
  int client_fd = response->fd_;
  while (true) {
    if (!response) {
      Schedule::Yield();
    }
    auto bytes = send(client_fd, response->buf_ + response->cursor_,
                      response->length_, 0);
    if (bytes > 0) {
      response->cursor_ += bytes;
      response->length_ -= bytes;
      Schedule::Yield();
    } else if (bytes == 0) {
      if (response->keep_alive_) {
        delete response;
        response = nullptr;
        request = new EventData();
        request->fd_ = client_fd;
        ControlEpollEvent(epfd, EPOLL_CTL_MOD, client_fd, EPOLLIN | EPOLLET,
                          request);
        Schedule::Yield();
      } else {
        delete response;
        response = nullptr;
        // don't keep alive
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_fd);
        close(client_fd);
        Schedule::Yield();
      }
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        Schedule::Yield();
      } else {
        ControlEpollEvent(epfd, EPOLL_CTL_DEL, client_fd);
        close(client_fd);
        delete response;
        response = nullptr;
        Schedule::Yield();
      }
    }
  }
}

void HttpServer::HandleRequestData(const EventData *request,
                                   EventData *response) {
  std::string request_str(request->buf_, request->length_);
  HttpRequest req;
  HttpResponse res;
  try {
    req.FromString(request_str);
    res = HandleRequestMessage(req);
  } catch (const std::invalid_argument &e) {
    res.SetStatusCode(HttpStatusCode::BadRequest);
    res.SetContent(e.what());
  } catch (const std::logic_error &e) {
    res.SetStatusCode(HttpStatusCode::HttpVersionNotSupported);
    res.SetContent(e.what());
  } catch (const std::exception &e) {
    res.SetStatusCode(HttpStatusCode::InternalServerError);
    res.SetContent(e.what());
  }
  std::string response_str = res.ToString(req.GetMethod() != HttpMethod::HEAD);
  memcpy(response->buf_, response_str.c_str(), response_str.length());
  response->length_ = response_str.length();

  auto connection = req.GetHeader("Connection");
  response->keep_alive_ = (connection == "keep-alive");

  // std::cout << "client: " << request->fd_ << ", request message: " <<
  // std::endl
  //           << request_str << std::endl;
  // std::cout << "client: " << request->fd_ << ", response message: " <<
  // std::endl
  //           << response_str << std::endl;
}

HttpResponse HttpServer::HandleRequestMessage(const HttpRequest &request) {
  auto path = request.GetPath();
  path = (path.find('?') == std::string::npos ? path
                                              : path.substr(0, path.find('?')));
  auto it = request_handlers_.find(path);
  if (it == request_handlers_.end()) {
    return HttpResponse(HttpStatusCode::NotFound);
  }
  auto callback = it->second.find(request.GetMethod());
  if (callback == it->second.end()) {
    return HttpResponse(HttpStatusCode::MethodNotAllowed);
  }
  return callback->second(request);
}

void HttpServer::ControlEpollEvent(int epfd, int op, int fd,
                                   std::uint32_t events, void *data) {
  // std::string msg;
  // if (events & EPOLLIN) {
  //   msg += "EPOLLIN ";
  // }
  // if (events & EPOLLOUT) {
  //   msg += "EPOLLOUT ";
  // }
  // std::cout << "[HttpServer::ControlEpollEvent] [epfd]: " << epfd
  //           << ", [op]: " << op << ", fd: " << fd << ", event: " << msg
  //           << std::endl;
  if (op == EPOLL_CTL_DEL) {
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
      SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
          << "[HttpServer::ControlEpollEvent] epoll_ctl failed: "
          << strerror(errno);
      throw std::runtime_error(
          "[HttpServer::ControlEpollEvent] epoll_ctl failed");
    }
  } else {
    epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.ptr = data;
    if (epoll_ctl(epfd, op, fd, &ev) < 0) {
      SYLAR_ERROR_LOG(SYLAR_LOG_ROOT)
          << "[HttpServer::ControlEpollEvent] epoll_ctl failed: "
          << strerror(errno);
      throw std::runtime_error(
          "[HttpServer::ControlEpollEvent] epoll_ctl failed");
    }
  }
}

} // namespace http

} // namespace Sylar