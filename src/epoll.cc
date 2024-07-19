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

void Epoll::RegisterEvent(int type, int fd, std::function<void()> r_func,
                          std::function<void()> w_func,
                          std::shared_ptr<Coroutine> r_co,
                          std::shared_ptr<Coroutine> w_co) {
  // set O_NONBLOCKING to fd
  auto state = fcntl(fd, F_GETFL);
  if ((state & O_NONBLOCK) == 0) {
    fcntl(fd, F_SETFL, state | O_NONBLOCK);
  }

  auto it = reg_event_.find(fd);
  if (it == reg_event_.end()) {
    EventCtx ctx;
    ctx.type_ = type;
    ctx.r_callback_.swap(r_func);
    ctx.w_callback_.swap(w_func);
    ctx.r_co_ = r_co;
    ctx.w_co_ = w_co;
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
    ev.events |= (EPOLLIN | EPOLLET);
    reg_event_[fd].type_ |= (int)EventType::READ;
  }
  if (type & EventType::WRITE) {
    ev.events |= (EPOLLOUT | EPOLLET);
    reg_event_[fd].type_ |= (int)EventType::WRITE;
  }
  epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

void Epoll::CancelEvent(int type, int fd) {
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
  ev.events |= (EPOLLIN | EPOLLOUT | EPOLLET);
  if (type & EventType::READ) {
    ev.events &= ~EPOLLIN;
    reg_event_[fd].type_ &= ~EventType::READ;
  }
  if (type & EventType::WRITE) {
    ev.events &= ~EPOLLOUT;
    reg_event_[fd].type_ &= ~EventType::WRITE;
  }
  if (reg_event_[fd].type_ == EventType::NONE) {
    reg_event_.erase(fd);
  }
  epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

} // namespace Sylar