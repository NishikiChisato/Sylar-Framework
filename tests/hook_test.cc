#include "../src/include/coroutine.hh"
#include "../src/include/hook.hh"
#include <gtest/gtest.h>
#include <thread>

TEST(Hook, WithHook) {
  auto attr = std::make_shared<Sylar::CoroutineAttr>();
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "start";
  auto co1 = Sylar::Coroutine::CreateCoroutine(
      Sylar::Schedule::GetThreadSchedule(), attr, [&]() {
        sleep(2);
        SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "sleep 2 seconds";
      });
  auto co2 = Sylar::Coroutine::CreateCoroutine(
      Sylar::Schedule::GetThreadSchedule(), attr, [&]() {
        sleep(3);
        SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "sleep 3 seconds";
      });
  co1->Resume();
  co2->Resume();

  std::thread t([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    Sylar::Schedule::GetThreadEpoll()->StopEventLoop(true);
  });

  Sylar::Schedule::Eventloop(Sylar::Schedule::GetThreadEpoll());

  t.join();

  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "stop";
}

TEST(Hook, WithoutHook) {
  Sylar::SetHookEnable(false);

  auto attr = std::make_shared<Sylar::CoroutineAttr>();
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "start";
  auto co1 = Sylar::Coroutine::CreateCoroutine(
      Sylar::Schedule::GetThreadSchedule(), attr, [&]() {
        sleep(2);
        SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "sleep 2 seconds";
      });
  auto co2 = Sylar::Coroutine::CreateCoroutine(
      Sylar::Schedule::GetThreadSchedule(), attr, [&]() {
        sleep(3);
        SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "sleep 3 seconds";
      });
  co1->Resume();
  co2->Resume();

  std::thread t([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    Sylar::Schedule::GetThreadEpoll()->StopEventLoop(true);
  });

  Sylar::Schedule::Eventloop(Sylar::Schedule::GetThreadEpoll());

  t.join();

  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "stop";
}