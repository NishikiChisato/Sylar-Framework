#include "include/epoll.hh"

namespace Sylar {

thread_local std::shared_ptr<Epoll> Epoll::t_epoll_ = nullptr;

std::shared_ptr<Epoll> Epoll::GetThreadEpoll() {
  if (!t_epoll_) {
    InitThreadEpoll();
  }
  return t_epoll_;
}

void Epoll::InitThreadEpoll() {
  t_epoll_ = Epoll::Instance();
  t_epoll_->epfd_ = epoll_create1(0);
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
  int op = 0;
  if (it == reg_event_.end()) {
    std::shared_ptr<EventCtx> ctx(new EventCtx);
    ctx->type_ = type;
    ctx->fd_ = fd;
    ctx->r_callback_.swap(r_func);
    ctx->w_callback_.swap(w_func);
    ctx->r_co_ = r_co;
    ctx->w_co_ = w_co;
    reg_event_[fd] = ctx;
    op = EPOLL_CTL_ADD;
  } else {
    if ((int)it->second->type_ & (int)type) {
      return;
    }
    reg_event_[fd]->type_ = type;
    reg_event_[fd]->r_callback_.swap(r_func);
    reg_event_[fd]->r_co_ = r_co;
    reg_event_[fd]->w_callback_.swap(w_func);
    reg_event_[fd]->w_co_ = w_co;
    op = EPOLL_CTL_MOD;
  }
  epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.fd = fd;
  if (type & EventType::READ) {
    ev.events |= (EPOLLIN | EPOLLET);
  }
  if (type & EventType::WRITE) {
    ev.events |= (EPOLLOUT | EPOLLET);
  }
  epoll_ctl(epfd_, op, fd, &ev);
}

void Epoll::RegisterEvent(int type, int fd, std::shared_ptr<EventCtx> ptr) {
  // set O_NONBLOCKING to fd
  auto state = fcntl(fd, F_GETFL);
  if ((state & O_NONBLOCK) == 0) {
    fcntl(fd, F_SETFL, state | O_NONBLOCK);
  }
  int op = EPOLL_CTL_ADD;
  if (reg_event_.find(fd) != reg_event_.end()) {
    op = EPOLL_CTL_MOD;
  }
  reg_event_[fd] = ptr;

  epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.ptr = ptr.get();
  printf("[Epoll::RegisterEvent] [fd]: %d, registing: %d\n", fd, type);
  if (type & EventType::READ) {
    ev.events |= (EPOLLIN | EPOLLET);
  }
  if (type & EventType::WRITE) {
    ev.events |= (EPOLLOUT | EPOLLET);
  }
  epoll_ctl(epfd_, op, fd, &ev);
}

void Epoll::CancelEvent(int type, int fd) {
  auto it = reg_event_.find(fd);
  if (it == reg_event_.end()) {
    return;
  } else {
    if ((it->second->type_ & type) == 0) {
      return;
    }
  }
  epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.ptr = (void *)reg_event_[fd].get();
  ev.events |= (EPOLLIN | EPOLLOUT | EPOLLET);
  if (type & EventType::READ) {
    ev.events &= ~EPOLLIN;
    reg_event_[fd]->type_ &= ~EventType::READ;
  }
  if (type & EventType::WRITE) {
    ev.events &= ~EPOLLOUT;
    reg_event_[fd]->type_ &= ~EventType::WRITE;
  }
  int op = EPOLL_CTL_MOD;
  if (reg_event_[fd]->type_ == EventType::NONE) {
    reg_event_.erase(fd);
    op = EPOLL_CTL_DEL;
  }
  epoll_ctl(epfd_, op, fd, &ev);
}

} // namespace Sylar