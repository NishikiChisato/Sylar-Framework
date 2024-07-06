#include "../src/include/coroutine.h"
#include "../src/include/iomanager.h"
#include "../src/include/schedule.h"
#include <chrono>
#include <filesystem>
#include <functional>
#include <gtest/gtest.h>
#include <random>
#include <set>
#include <string>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <thread>
#include <vector>

void WriteFunc(int fd, int cnt) {
  for (int i = 0; i < cnt; i++) {
    char buf[256];
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%d", i);
    write(fd, buf, sizeof(buf));
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

void ReadFunc(int fd) {
  char buf[256];
  memset(buf, 0, sizeof(buf));
  std::string str;
  while (read(fd, buf, sizeof(buf)) > 0) {
    str.append(buf);
  }
  std::cout << str << std::endl;
}

void BasicTsk(int *fifo) {
  fcntl(fifo[0], F_SETFL, O_NONBLOCK);
  fcntl(fifo[1], F_SETFL, O_NONBLOCK);
  Sylar::IOManager::GetThis()->AddEvent(fifo[0], Sylar::IOManager::Event::READ,
                                        std::bind(&ReadFunc, fifo[0]));
  Sylar::IOManager::GetThis()->AddEvent(
      fifo[1], Sylar::IOManager::Event::WRITE,
      []() { std::cout << "read event tirgger" << std::endl; });

  std::thread t1([&]() { WriteFunc(fifo[1], 10); });
  t1.join();
}

TEST(IOManager, Basic) {
  Sylar::IOManager iomgr(1, true);
  int fifo[2];
  pipe(fifo);
  iomgr.ScheduleTask(std::bind(&BasicTsk, fifo));
  iomgr.Stop();

  close(fifo[0]);
  close(fifo[1]);
}

Sylar::Mutex read_mu;
uint32_t read_cnt = 0;
Sylar::Mutex write_mu;
uint32_t write_cnt = 0;

void ReadEvent() {
  Sylar::Mutex::ScopeLock l(read_mu);
  read_cnt++;
}

void WriteEvent() {
  Sylar::Mutex::ScopeLock l(write_mu);
  write_cnt++;
}

// this function simulate 5000 socket connection
void ManyTask(int threads, bool use_caller) {
  read_cnt = 0;
  write_cnt = 0;
  Sylar::IOManager::ptr iomgr(new Sylar::IOManager(threads, use_caller));
  const uint32_t sock_num = 500;
  int sock[sock_num][2];
  memset(sock, 0, sizeof(sock));
  for (int i = 0; i < sock_num; i++) {
    pipe(sock[i]);
    fcntl(sock[i][0], F_SETFL, O_NONBLOCK);
    fcntl(sock[i][1], F_SETFL, O_NONBLOCK);
  }

  for (int i = 0; i < sock_num; i++) {
    iomgr->AddEvent(sock[i][0], Sylar::IOManager::Event::READ, &ReadEvent);
    iomgr->AddEvent(sock[i][1], Sylar::IOManager::Event::WRITE, &WriteEvent);
  }

  std::atomic<bool> stop{false};

  std::thread t1([&]() {
    while (!stop.load()) {
      for (int i = 0; i < sock_num; i++) {
        write(sock[i][1], "T", 1);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }
    }
  });

  iomgr->Stop();

  stop.store(true);

  t1.join();

  // we cannot calculate the number of execution of callback, we only guarantee
  // that it at least execute once
  EXPECT_GE(read_cnt, sock_num);
  EXPECT_GE(write_cnt, sock_num);
  std::cout << "read cnt: " << read_cnt << ", write cnt: " << write_cnt
            << std::endl;

  for (int i = 0; i < sock_num; i++) {
    close(sock[i][0]);
    close(sock[i][1]);
  }
}

TEST(IOManager, ManyTaskSingleThreadUseCaller) { ManyTask(1, true); }

TEST(IOManager, ManyTaskSingleThreadNotUseCaller) { ManyTask(1, false); }

TEST(IOManager, ManyTaskMultiThreadUseCaller) { ManyTask(25, true); }

TEST(IOManager, ManyTaskMultiThreadNotUseCaller) { ManyTask(25, false); }

TEST(IOManager, CancelEvent) {
  Sylar::IOManager::ptr iomgr(new Sylar::IOManager());
  iomgr->AddEvent(0, Sylar::IOManager::READ,
                  []() { std::cout << "read event 1 at 0" << std::endl; });

  iomgr->AddEvent(0, Sylar::IOManager::WRITE,
                  []() { std::cout << "write event 2 at 0" << std::endl; });

  iomgr->CancelEvent(0, Sylar::IOManager::READ);
  iomgr->CancelEvent(0, Sylar::IOManager::WRITE);

  iomgr->Stop();
}

TEST(IOManager, DeleteEvent) {
  Sylar::IOManager::ptr iomgr(new Sylar::IOManager());
  iomgr->AddEvent(0, Sylar::IOManager::READ,
                  []() { std::cout << "read event 1 at 0" << std::endl; });

  iomgr->AddEvent(0, Sylar::IOManager::WRITE,
                  []() { std::cout << "write event 2 at 0" << std::endl; });

  std::cout << iomgr->ListAllEvent() << std::endl;

  iomgr->DelEvent(0, Sylar::IOManager::READ);
  iomgr->DelEvent(0, Sylar::IOManager::WRITE);

  std::cout << iomgr->ListAllEvent() << std::endl;
}

TEST(IOManager, TimerBasic) {
  Sylar::IOManager::ptr iomgr(new Sylar::IOManager());
  auto now = Sylar::GetElapseFromRebootMS();
  std::cout << "now = " << now << "ms" << std::endl;
  iomgr->AddTimer(
      "timer_1", 5000,
      [&]() {
        uint64_t tri = Sylar::GetElapseFromRebootMS();
        std::cout << "timer_1 trigger time = " << tri
                  << "ms, distence = " << tri - now << std::endl;
      },
      false);

  iomgr->AddTimer(
      "timer_2", 1000,
      [&]() {
        uint64_t tri = Sylar::GetElapseFromRebootMS();
        std::cout << "timer_2 trigger time = " << tri
                  << "ms, distence = " << tri - now << std::endl;
      },
      false);

  iomgr->AddTimer(
      "timer_3", 2500,
      [&]() {
        uint64_t tri = Sylar::GetElapseFromRebootMS();
        std::cout << "timer_3 trigger time = " << tri
                  << "ms, distence = " << tri - now << std::endl;
      },
      false);

  iomgr->Stop();
}

TEST(IOManager, NoHook) {
  Sylar::IOManager::ptr iom(new Sylar::IOManager(1, false));
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