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