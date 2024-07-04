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

void BasicTsk(int *pp) {
  fcntl(pp[0], F_SETFL, O_NONBLOCK);
  fcntl(pp[1], F_SETFL, O_NONBLOCK);
  Sylar::IOManager::GetThis()->AddEvent(pp[0], Sylar::IOManager::Event::READ,
                                        std::bind(&ReadFunc, pp[0]));
  Sylar::IOManager::GetThis()->AddEvent(
      pp[1], Sylar::IOManager::Event::WRITE,
      []() { std::cout << "read event tirgger" << std::endl; });

  std::thread t1([&]() { WriteFunc(pp[1], 10); });
  t1.join();
}

TEST(IOManager, Basic) {
  Sylar::IOManager iomgr(1, true);
  int pp[2];
  pipe(pp);
  iomgr.ScheduleTask(std::bind(&BasicTsk, pp));
  iomgr.Stop();

  close(pp[0]);
  close(pp[1]);
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