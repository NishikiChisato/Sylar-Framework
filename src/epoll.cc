#include "include/epoll.hh"

namespace Sylar {

int Epoll::epfd_ = -1;
std::unordered_map<int, Epoll::EventCtx> Epoll::reg_event_;
bool Epoll::loop_ = false;

void Epoll::InitEpoll(std::shared_ptr<Epoll> ep) {
  if (!ep) {
    return;
  }
  ep->epfd_ = epoll_create(1);
}

void Epoll::RegisterEvent(EventType type, int fd, std::function<void()> func) {
  // set O_NONBLOCKING to fd
  auto state = fcntl(fd, F_GETFL);
  if ((state & O_NONBLOCK) == 0) {
    fcntl(fd, F_SETFL, state | O_NONBLOCK);
  }

  auto it = reg_event_.find(fd);
  if (it == reg_event_.end()) {
    EventCtx ctx;
    ctx.type_ = type;
    ctx.callback_.swap(func);
    reg_event_[fd] = ctx;
  } else {
    if ((int)it->second.type_ & (int)type) {
      return;
    }
  }
  epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.ptr = &reg_event_[fd];
  if (type & EventType::READ) {
    ev.events |= EPOLLIN;
    it->second.type_ |= (int)EventType::READ;
  }
  if (type & EventType::WRITE) {
    ev.events |= EPOLLOUT;
    it->second.type_ |= (int)EventType::WRITE;
  }
  epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

void Epoll::CancelEvent(EventType type, int fd) {
  auto it = reg_event_.find(fd);
  if (it == reg_event_.end()) {
    return;
  } else {
    if ((it->second.type_ & type) == 0) {
      return;
    }
  }
  epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.ptr = &reg_event_[fd];
  if (type & EventType::READ) {
    ev.events |= EPOLLIN;
    it->second.type_ &= ~EventType::READ;
  }
  if (type & EventType::WRITE) {
    ev.events |= EPOLLOUT;
    it->second.type_ &= ~EventType::WRITE;
  }
  epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

} // namespace Sylar