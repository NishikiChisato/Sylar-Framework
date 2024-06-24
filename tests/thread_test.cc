#include "../src/include/config.h"
#include "../src/include/log.h"
#include "../src/include/mutex.h"
#include "../src/include/thread.h"
#include <filesystem>
#include <functional>
#include <gtest/gtest.h>
#include <string>
#include <thread>

void func1() {
  for (int i = 0; i < 1000; i++) {
    SYLAR_INFO_LOG(SYLAR_LOG_NAME("system")) << "i = " << i;
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
}

void Remove() {
  for (int i = 0; i < 10; i++) {
    while (!std::filesystem::exists("./logs/root.log"))
      ;
    std::remove("./logs/root.log");
    std::this_thread::sleep_for(std::chrono::microseconds(233));
  }
}

TEST(Thread, SupportForLogModule) {
  YAML::Node node = YAML::LoadFile("../conf/log.yaml");
  Sylar::Config::LoadFromYaml(node);
  auto p1 = Sylar::Config::Lookup("logs", std::set<Sylar::LoggerConf>(), "");

  std::cout << p1->ToString() << std::endl;
  Sylar::LoggerMgr::GetInstance()->GetRoot()->CheckAppenders();

  std::function<void()> f1 = &func1, f2 = &func1, f3 = &Remove;
  Sylar::Thread t1(f1, "t1"), t2(f2, "t2"), t3(f3, "t3");
  t1.Join();
  t2.Join();
  t3.Join();
  std::cout << "the number of line: " << std::endl;
  system("wc -l ./logs/root.log");
  std::filesystem::remove("./logs/root.log");
  std::filesystem::remove("./logs");
}

int g_cnt = 0, g_all = 1000;
Sylar::Mutex mu;

void func2() {
  Sylar::Mutex::ScopeLock l(mu);
  std::cout << Sylar::Thread::GetName() << " thread is running" << std::endl;
  for (int i = 0; i < g_all; i++) {
    g_cnt++;
  }
}

void func3() {
  Sylar::Mutex::ScopeLock l(mu);
  std::cout << Sylar::Thread::GetName() << " thread is running" << std::endl;
  for (int i = 0; i < g_all; i++) {
    g_cnt++;
  }
}

void func4(int x) {
  Sylar::Mutex::ScopeLock l(mu);
  std::cout << Sylar::Thread::GetName() << " thread is running" << std::endl;
  for (int i = 0; i < x; i++) {
    g_cnt++;
  }
}

TEST(Thread, Usage) {
  // basic usage
  Sylar::Thread::ptr p1(new Sylar::Thread(&func2, "t1"));
  Sylar::Thread::ptr p2(new Sylar::Thread(&func3, "t2"));
  p1->Join();
  p2->Join();
  EXPECT_EQ(g_cnt, 2 * g_all);

  // use std::bind
  int cnt = 1000;
  std::function<void(int)> f = &func4;
  auto b = std::bind(f, cnt);
  Sylar::Thread::ptr p3(new Sylar::Thread(b, "t3"));

  p3->Join();
  EXPECT_EQ(g_cnt, 2 * g_all + cnt);
}