#ifndef __SYLAR_TCPSERVER_H__
#define __SYLAR_TCPSERVER_H__

#include "coroutine.hh"
#include "http_message.hh"
#include "log.h"
#include <algorithm>
#include <arpa/inet.h>
#include <condition_variable>
#include <mutex>
#include <netinet/ip.h>
#include <random>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unordered_map>

namespace Sylar {

namespace http {

static constexpr int kMaxBufferSize = 4096;

struct EventData {
  EventData() : fd_(-1), length_(0), cursor_(0), buf_() {}
  int fd_;
  size_t length_; // the length of received data
  size_t cursor_; // when we want to send response message, this cursor point to
                  // last bytes to send to client
  char buf_[kMaxBufferSize]; // buffer to receive http request message and
                             // store http response message
};

using HttpRequestHandler = std::function<HttpResponse(const HttpRequest &)>;

class HttpServer {
public:
  using ptr = std::shared_ptr<HttpServer>;

  HttpServer() = default;
  explicit HttpServer(const std::string &host, std::uint16_t port);
  ~HttpServer() = default;

  void Start();
  void Stop();

  void RegisterHttpRequestHandler(const std::string &path, HttpMethod method,
                                  const HttpRequestHandler handler) {
    request_handlers_[path].insert(std::make_pair(method, std::move(handler)));
  }

  std::string GetHost() const { return host_; }
  std::uint16_t GetPort() const { return port_; }
  bool Running() const { return running_; }

private:
  void ListenClient();

  /**
   * @brief each thread will execute this function, periodically invoke
   * epoll_wait. the logic of processing READ or WRITE event in
   * HandleEpollEvent.
   *
   * @param thread the index of thread
   */
  void EventLoop(int thread);

  /**
   * @brief process READ or WRITE event, including:
   *
   * receive http request message from client
   * registering WRITE event in epoll after receiving
   * generating http response message, which is responsible by HandleRequestData
   * registering READ event after write whole http response message to client.
   */
  void HandleEpollEvent(int epfd, std::uint32_t event, EventData *data);

  void HandleRequestData(const EventData *request, EventData *response);

  HttpResponse HandleRequestMessage(const HttpRequest &request);

  void ControlEpollEvent(int epfd, int op, int fd, std::uint32_t events = 0,
                         void *data = nullptr);

private:
  static constexpr std::size_t kThreadPoolSize = 5;
  static constexpr std::size_t kMaxEvent = 10240;
  static constexpr std::uint64_t kMaxEpollWaitTimeout = 1000; // 1 second

  std::string host_;
  std::uint16_t port_;
  int sock_fd_; // this socket serve as accpet socket
  bool running_;

  std::thread listener_thread_;
  std::vector<std::thread> worker_threads_; // size is kThreadPoolSize
  std::vector<int> worker_epoll_fd_;        // size is kThreadPoolSize
  std::vector<std::vector<epoll_event>>
      worker_epoll_event_; // size is kThreadPoolSize(row) * KMaxEvent(col)

  std::random_device rdev_;
  std::mt19937 random_engine_;
  std::uniform_int_distribution<int> sleep_time_; // range from [10， 100]

  // [path, method, callback function]
  std::map<std::string, std::map<HttpMethod, HttpRequestHandler>>
      request_handlers_;
};

} // namespace http

} // namespace Sylar

#endif