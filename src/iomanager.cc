#include "include/iomanager.h"

namespace Sylar {

IOManager::FDContext::EventContext &
IOManager::FDContext::GetEventContext(IOManager::Event event) {
  switch (event) {
  case Event::READ:
    return read_ctx_;
  case Event::WRITE:
    return write_ctx_;
  default:
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "IOManager::FDContext::GetEventContext invalid arguement";
    throw std::invalid_argument(
        "IOManager::FDContext::GetEventContext invalid arguement");
  }
}

void IOManager::FDContext::ResetEventContext(IOManager::Event event) {
  switch (event) {
  case Event::READ:
    read_ctx_.coroutine_.reset();
    read_ctx_.func_ = nullptr;
    read_ctx_.scheduler_ = nullptr;
    break;
  case Event::WRITE:
    write_ctx_.coroutine_.reset();
    write_ctx_.func_ = nullptr;
    write_ctx_.scheduler_ = nullptr;
    break;
  default:
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "IOManager::FDContext::ResetEventContext invalid arguement: "
        << event;
  }
}

void IOManager::FDContext::TriggerEvent(IOManager::Event event) {
  EventContext &ectx = GetEventContext(event);
  if (!ectx.scheduler_) {
    return;
  }
  /**
   * if we want to clear/unset bits, we use AND and NOT:
   * flag &= ~opt
   * it will unset all bits
   */
  event_ = (Event)(event_ & ~event);
  if (ectx.func_) {
    ectx.scheduler_->ScheduleTask(ectx.func_);
  } else {
    ectx.scheduler_->ScheduleTask(ectx.coroutine_);
  }
}

std::string IOManager::EventToString(Event event) {
  switch (event) {

#define XX(str)                                                                \
  case Event::str:                                                             \
    return #str;

    XX(NONE)
    XX(READ)
    XX(WRITE)

#undef XX
  }
}

void IOManager::ExtendFDContext(size_t sz) {
  if (sz < fd_context_.size()) {
    return;
  }
  fd_context_.resize(sz);
  for (size_t i = 0; i < sz; i++) {
    fd_context_[i] = new FDContext();
    fd_context_[i]->fd_ = i;
  }
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Schedule(threads, use_caller, name), TimeManager() {
  epfd_ = epoll_create1(0);

  int ret = pipe(notify_pipe_);
  SYLAR_ASSERT(!ret);

  epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = EPOLLIN | EPOLLET;
  // we save the read side of this pipe
  event.data.fd = notify_pipe_[0];

  // besides, we must set the read side to non-block
  int flag = fcntl(notify_pipe_[0], F_GETFL, 0);
  ret = fcntl(notify_pipe_[0], F_SETFL, flag | O_NONBLOCK);
  SYLAR_ASSERT(!ret);

  ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, notify_pipe_[0], &event);
  SYLAR_ASSERT(!ret);

  ExtendFDContext(32);

  Start();
}

IOManager::~IOManager() {
  close(epfd_);
  close(notify_pipe_[0]);
  close(notify_pipe_[1]);
  for (int i = 0; i < fd_context_.size(); i++) {
    delete fd_context_[i];
  }
}

bool IOManager::AddEvent(int fd, IOManager::Event event,
                         std::function<void()> func) {
  SYLAR_ASSERT(fd >= 0);
  FDContext *fdctx = nullptr;
  Mutex::ScopeLock l1(mu_);
  // extend vector size if necessary
  if (fd >= fd_context_.size()) {
    ExtendFDContext(2 * fd);
  }
  fdctx = fd_context_[fd];
  l1.Unlock();

  Mutex::ScopeLock l2(fdctx->mu_);
  // we cannot add duplicate event to fd
  if (fdctx->event_ & event) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT) << "IOManager::AddEvent event duplicated";
    return false;
  }
  int op = fdctx->event_ ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event ep_event;
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = fdctx->event_ | event | EPOLLET;
  // the data field in epoll_event can used to store additional information, in
  // this case, we save the pointer of context of fd
  ep_event.data.ptr = fdctx;

  int ret = epoll_ctl(epfd_, op, fdctx->fd_, &ep_event);
  if (ret) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "IOManager::AddEvent epoll_ctl error: " << strerror(errno);
    return false;
  }

  // we must assign callback function to that fd

  // first, we try to get read context
  if (event & Event::READ) {
    auto &rctx = fdctx->GetEventContext((Event)(event & Event::READ));
    rctx.func_ = func;
    rctx.scheduler_ = dynamic_cast<Schedule *>(this);
    SYLAR_ASSERT(rctx.scheduler_);
    pending_event_++;
  }
  if (event & Event::WRITE) {
    auto &wctx = fdctx->GetEventContext((Event)(event & Event::WRITE));
    wctx.func_ = func;
    wctx.scheduler_ = dynamic_cast<Schedule *>(this);
    SYLAR_ASSERT(wctx.scheduler_);
    pending_event_++;
  }
  fdctx->event_ = (Event)(fdctx->event_ | event);
  return true;
}

bool IOManager::AddEvent(int fd, IOManager::Event event,
                         Coroutine::ptr coroutine) {
  SYLAR_ASSERT(fd >= 0);
  FDContext *fdctx = nullptr;
  Mutex::ScopeLock l1(mu_);
  // extend vector size if necessary
  if (fd >= fd_context_.size()) {
    ExtendFDContext(2 * fd);
  }
  fdctx = fd_context_[fd];
  l1.Unlock();

  Mutex::ScopeLock l2(fdctx->mu_);
  // we cannot add duplicate event to fd
  if (fdctx->event_ & event) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT) << "IOManager::AddEvent event duplicated";
    return false;
  }
  int op = fdctx->event_ ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event ep_event;
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = fdctx->event_ | event | EPOLLET;
  // the data field in epoll_event can used to store additional information, in
  // this case, we save the pointer of context of fd
  ep_event.data.ptr = fdctx;

  int ret = epoll_ctl(epfd_, op, fdctx->fd_, &ep_event);
  if (ret) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "IOManager::AddEvent epoll_ctl error: " << strerror(errno);
    return false;
  }

  // we must assign callback function to that fd

  // first, we try to get read context
  if (event & Event::READ) {
    auto &rctx = fdctx->GetEventContext((Event)(event & Event::READ));
    rctx.coroutine_ = coroutine;
    rctx.scheduler_ = dynamic_cast<Schedule *>(this);
    SYLAR_ASSERT(rctx.scheduler_);
    pending_event_++;
  }
  if (event & Event::WRITE) {
    auto &wctx = fdctx->GetEventContext((Event)(event & Event::WRITE));
    wctx.coroutine_ = coroutine;
    wctx.scheduler_ = dynamic_cast<Schedule *>(this);
    SYLAR_ASSERT(wctx.scheduler_);
    pending_event_++;
  }
  fdctx->event_ = (Event)(fdctx->event_ | event);
  return true;
}

bool IOManager::DelEvent(int fd, IOManager::Event event) {
  Mutex::ScopeLock l1(mu_);
  if (fd >= fd_context_.size()) {
    return false;
  }
  FDContext *fdctx = fd_context_[fd];
  l1.Unlock();

  Mutex::ScopeLock l2(fdctx->mu_);

  Event left_event = (Event)(fdctx->event_ & ~event);
  int op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event ep_event;
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = (int)left_event | EPOLLET;
  ep_event.data.ptr = fdctx;
  int ret = epoll_ctl(epfd_, op, fd, &ep_event);
  if (ret) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "IOManager::DelEvent epoll_ctl error: " << strerror(errno);
    return false;
  }

  fdctx->event_ = left_event;
  if (fdctx->event_ & Event::READ) {
    fdctx->ResetEventContext(Event::READ);
    pending_event_--;
  }
  if (fdctx->event_ & Event::WRITE) {
    fdctx->ResetEventContext(Event::WRITE);
    pending_event_--;
  }
  return true;
}

bool IOManager::CancelEvent(int fd, IOManager::Event event) {
  Mutex::ScopeLock l1(mu_);
  if (fd >= fd_context_.size()) {
    return false;
  }
  FDContext *fdctx = fd_context_[fd];
  l1.Unlock();

  if (!(fdctx->event_ & event)) {
    return true;
  }

  Mutex::ScopeLock l2(fdctx->mu_);

  Event left_event = (Event)(fdctx->event_ & ~event);
  int op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event ep_event;
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.events = (int)left_event | EPOLLET;
  ep_event.data.ptr = fdctx;
  int ret = epoll_ctl(epfd_, op, fd, &ep_event);
  if (ret) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "IOManager::CancelEvent epoll_ctl error: " << strerror(errno);
    return false;
  }

  // before we reset event, we should trigger callback function

  if (fdctx->event_ & Event::READ) {
    fdctx->TriggerEvent(Event::READ);
    fdctx->ResetEventContext(Event::READ);
    pending_event_--;
  }
  if (fdctx->event_ & Event::WRITE) {
    fdctx->TriggerEvent(Event::WRITE);
    fdctx->ResetEventContext(Event::WRITE);
    pending_event_--;
  }
  return true;
}

bool IOManager::CancelAllEvent(int fd) {
  Mutex::ScopeLock l1(mu_);
  if (fd >= fd_context_.size()) {
    return false;
  }
  FDContext *fdctx = fd_context_[fd];
  l1.Unlock();

  Mutex::ScopeLock l2(fdctx->mu_);

  int op = EPOLL_CTL_DEL;
  epoll_event ep_event;
  memset(&ep_event, 0, sizeof(ep_event));
  ep_event.data.ptr = fdctx;
  int ret = epoll_ctl(epfd_, op, fd, &ep_event);
  if (ret) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "IOManager::CancelAllEvent epoll_ctl error: " << strerror(errno);
    return false;
  }

  // before we reset event, we should trigger callback function
  if (fdctx->event_ & Event::READ) {
    fdctx->TriggerEvent(Event::READ);
    fdctx->ResetEventContext(Event::READ);
    pending_event_--;
  }
  if (fdctx->event_ & Event::WRITE) {
    fdctx->TriggerEvent(Event::WRITE);
    fdctx->ResetEventContext(Event::WRITE);
    pending_event_--;
  }
  return true;
}

std::string IOManager::ListAllEvent() {
  std::stringstream ss;
  for (auto &it : fd_context_) {
    if (it && it->event_ != NONE) {
      std::stringstream str;
      str << "[" << it->fd_ << "]: ";
      if (it->event_ & READ) {
        str << " READ ";
      }
      if (it->event_ & WRITE) {
        str << " WRITE ";
      }
      ss << str.str();
    }
  }
  return ss.str();
}

void IOManager::Notify() { write(notify_pipe_[1], "$", 1); }

void IOManager::Idel() {
  const int64_t MAX_EVENTS = 256;      // maxevents arguement in epoll_wait
  const uint64_t EPOLL_TIMEOUT = 1000; // timeout arguement in epoll_wait
  uint64_t nxt_timeout = EPOLL_TIMEOUT;
  epoll_event *events = new epoll_event[MAX_EVENTS];
  memset(events, 0, sizeof(epoll_event) * MAX_EVENTS);
  std::shared_ptr<epoll_event> defer_events(events,
                                            [](epoll_event *p) { delete[] p; });
  while (!IsStop()) {
    int64_t now = GetElapseFromRebootMS();
    nxt_timeout = GetNextTriggerTime(now);
    if (nxt_timeout != ~0ull) {
      nxt_timeout = std::max(10ul, std::min(nxt_timeout, EPOLL_TIMEOUT));
    } else {
      nxt_timeout = EPOLL_TIMEOUT;
    }

    // we should block in epoll_wait
    int cnt = epoll_wait(epfd_, events, MAX_EVENTS, nxt_timeout);
    if (cnt <= 0) {
      /**
       * when epoll_wait return -1 and set errno with EAGAIN or EWOULDBLOCK
       *
       * it means that there are no events available, it's common in
       * non-blocking
       */
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        // SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "IOManager::Idel epoll_wait
        // encounter error: " << strerror(errno);
        if (HasExpired()) {
          std::vector<std::function<void()>> vfunc;
          GetAllExpired(GetElapseFromRebootMS(), vfunc);
          if (!vfunc.empty()) {
            for (auto &it : vfunc) {
              ScheduleTask(it);
            }
          }
        }

        /**
         * may be we have to execute task or timer event, we should exit idel
         * coroutine
         */
        Coroutine::YieldToHold();
      }
    }
    for (int i = 0; i < cnt; i++) {
      auto &event = events[i];
      // the entry with index 0 is notify pipe, we should consume it first
      if (event.data.fd == notify_pipe_[0]) {
        char buf[256];
        while (read(notify_pipe_[0], buf, sizeof(buf)) > 0)
          ;
        // if only notify_pipe has content, we should exit idel to execute task
        if (cnt == 1) {
          Coroutine::YieldToHold();
        }
        continue;
      }
      // now, we extract event and set outself's Event
      FDContext *fdptr = (FDContext *)event.data.ptr;
      SYLAR_ASSERT(fdptr);
      Mutex::ScopeLock l(fdptr->mu_);

      int real_event = Event::NONE;
      if (event.events & EPOLLIN) {
        real_event |= Event::READ;
      }
      if (event.events & EPOLLOUT) {
        real_event |= Event::WRITE;
      }
      int rest_event = fdptr->event_ & ~real_event;
      int op = rest_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = EPOLLET | rest_event;
      int rt = epoll_ctl(epfd_, op, fdptr->fd_, &event);
      if (rt) {
        SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
            << "IOManager::Idel epoll_ctl encounter error: " << strerror(errno)
            << ", fd: " << fdptr->fd_;
      }

      if ((real_event & Event::READ) && (fdptr->event_ & Event::READ)) {
        fdptr->TriggerEvent(Event::READ);
        fdptr->ResetEventContext(Event::READ);
        pending_event_--;
      }
      if ((real_event & Event::WRITE) && (fdptr->event_ & Event::WRITE)) {
        fdptr->TriggerEvent(Event::WRITE);
        fdptr->ResetEventContext(Event::WRITE);
        pending_event_--;
      }
    }
  }

  Coroutine::YieldToHold();
}

} // namespace Sylar