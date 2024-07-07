#include "../src/include/coroutine.h"
#include "../src/include/hook.h"
#include "../src/include/iomanager.h"
#include "../src/include/schedule.h"
#include <gtest/gtest.h>

TEST(Hook, WithHook) {
  Sylar::IOManager::ptr iom(new Sylar::IOManager(1, true));
  iom->Start();
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "start";
  iom->ScheduleTask([]() {
    sleep(2);
    SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "sleep 2 seconds";
  });
  iom->ScheduleTask([]() {
    sleep(3);
    SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "sleep 3 seconds";
  });
  iom->Stop();
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "stop";
}

TEST(Hook, WithoutHook) {
  Sylar::SetHookEnable(false);
  Sylar::IOManager::ptr iom(new Sylar::IOManager(1, true));
  iom->Start();
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "start";
  iom->ScheduleTask([]() {
    sleep(2);
    SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "sleep 2 seconds";
  });
  iom->ScheduleTask([]() {
    sleep(3);
    SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "sleep 3 seconds";
  });
  iom->Stop();
  SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "stop";
}