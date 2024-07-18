#include "../src/include/coroutine.hh"
#include <gtest/gtest.h>
#include <thread>

void Swap() { Sylar::Schedule::Yield(); }

void tsk1(uint64_t st) {
  auto ed = Sylar::GetElapseFromRebootMS();
  std::cout << "tsk1 execute at: " << ed << ", dis = " << ed - st << std::endl;
}

TEST(Epoll, TimerEvent) {
  auto attr = std::make_shared<Sylar::CoroutineAttr>();
  attr->shared_mem_ = Sylar::SharedMem::AllocSharedMem(4, 64 * 1024);

  auto co1 = Sylar::Coroutine::CreateCoroutine(
      Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Swap));
  auto co2 = Sylar::Coroutine::CreateCoroutine(
      Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Swap));

  auto st = Sylar::GetElapseFromRebootMS();

  std::cout << "start: " << st << std::endl;

  auto epoll = Sylar::Schedule::GetThreadEpoll();

  std::thread t1([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    epoll->StopEventLoop(true);
  });

  epoll->AddTimer(100, std::bind(&tsk1, st));
  epoll->AddTimer(200, std::bind(&tsk1, st));
  epoll->AddTimer(500, std::bind(&tsk1, st));
  epoll->AddTimer(1000, std::bind(&tsk1, st));
  epoll->AddTimer(2000, std::bind(&tsk1, st));

  Sylar::Schedule::Eventloop(Sylar::Schedule::GetThreadEpoll());

  t1.join();
}

void tsk2(uint64_t st) {
  Sylar::Schedule::Yield();
  auto ed = Sylar::GetElapseFromRebootMS();
  std::cout << "tsk1 execute at: " << ed << ", dis = " << ed - st << std::endl;
}

TEST(Epoll, CoroutineTime) {
  auto attr = std::make_shared<Sylar::CoroutineAttr>();
  attr->shared_mem_ = Sylar::SharedMem::AllocSharedMem(1, 64 * 1024);

  auto st = Sylar::GetElapseFromRebootMS();

  std::cout << "start: " << st << std::endl;

  auto co1 = Sylar::Coroutine::CreateCoroutine(
      Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&tsk2, st));

  co1->Resume();

  auto epoll = Sylar::Schedule::GetThreadEpoll();
  epoll->AddTimer(100, nullptr, co1);
  epoll->StopEventLoop(false);

  std::thread t1([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    epoll->StopEventLoop(true);
  });

  Sylar::Schedule::Eventloop(epoll);

  t1.join();
}