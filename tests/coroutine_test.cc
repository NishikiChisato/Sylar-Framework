#include "../src/include/coroutine.hh"
#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <thread>

TEST(Coroutine, MemoryAlloc) {
  auto v1 = Sylar::MemoryAlloc::AllocSharedMemory(128);
  memset(v1.get(), 0xff, sizeof(128));
  for (int i = 0; i < 128; i++) {
    EXPECT_EQ(*reinterpret_cast<uint8_t *>(v1.get()), 0xff);
  }
}

void swap() { Sylar::Schedule::Yield(); }

TEST(Coroutine, BasicSwap1) {
  {
    std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
    auto co1 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&swap));
    co1->Resume();
    EXPECT_EQ(co1->GetCoState(), Sylar::Coroutine::CO_READY);
  }
}

TEST(Coroutine, BasicSwap2) {
  {
    std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
    auto co1 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&swap));
    co1->Resume();
    co1->Resume();
    EXPECT_EQ(co1->GetCoState(), Sylar::Coroutine::CO_TERMINAL);
  }
}

int invoke_cnt = 0;

void Invoke3() {
  invoke_cnt++;
  Sylar::Schedule::Yield();
}

void Invoke2() {
  {
    invoke_cnt++;
    std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
    auto co1 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Invoke3));
    co1->Resume();
    Sylar::Schedule::Yield();
  }
}

void Invoke1() {
  {
    invoke_cnt++;
    std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
    auto co1 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Invoke2));
    co1->Resume();
    Sylar::Schedule::Yield();
  }
}

TEST(Coroutine, RecursiveInvoke1) {
  {
    invoke_cnt = 0;
    std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
    auto co1 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Invoke1));
    co1->Resume();
    EXPECT_EQ(co1->GetCoState(), Sylar::Coroutine::CO_READY);
    EXPECT_EQ(invoke_cnt, 3);
  }
}

TEST(Coroutine, RecursiveInvoke2) {
  {
    invoke_cnt = 0;
    std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
    auto co1 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Invoke1));
    co1->Resume();
    co1->Resume();
    EXPECT_EQ(co1->GetCoState(), Sylar::Coroutine::CO_TERMINAL);
    EXPECT_EQ(invoke_cnt, 3);
  }
}

TEST(Coroutine, SharedMemoryBasic) {
  {
    std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
    attr->shared_mem_ = Sylar::SharedMem::AllocSharedMem(1, 128 * 1024);
    auto co1 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&swap));
    auto co2 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&swap));
    co1->Resume();
    co2->Resume();
    co1->Resume();
    co2->Resume();
    EXPECT_EQ(co1->GetCoState(), Sylar::Coroutine::CO_TERMINAL);
    EXPECT_EQ(co2->GetCoState(), Sylar::Coroutine::CO_TERMINAL);
  }
}

TEST(Coroutine, SharedMemoryRecursiveInvoke) {
  {
    invoke_cnt = 0;
    std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
    attr->shared_mem_ = Sylar::SharedMem::AllocSharedMem(1, 128 * 1024);
    auto co1 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Invoke1));
    auto co2 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Invoke1));
    co1->Resume();
    co2->Resume();
    co1->Resume();
    co2->Resume();
    EXPECT_EQ(co1->GetCoState(), Sylar::Coroutine::CO_TERMINAL);
    EXPECT_EQ(co2->GetCoState(), Sylar::Coroutine::CO_TERMINAL);
    EXPECT_EQ(invoke_cnt, 6);
  }
}

void MemoryCheckInvoke3() { Sylar::Schedule::Yield(); }

void MemoryChecKInvoke2() {
  const int sz = 128;
  char *buf = (char *)malloc(sz);
  memset(buf, 0xff, sz);

  std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
  attr->shared_mem_ = Sylar::SharedMem::AllocSharedMem(1, 128 * 1024);
  auto co1 =
      Sylar::Coroutine::CreateCoroutine(Sylar::Schedule::GetThreadSchedule(),
                                        attr, std::bind(&MemoryCheckInvoke3));
  auto co2 =
      Sylar::Coroutine::CreateCoroutine(Sylar::Schedule::GetThreadSchedule(),
                                        attr, std::bind(&MemoryCheckInvoke3));
  co1->Resume();
  co2->Resume();

  for (int i = 0; i < sz; i++) {
    EXPECT_EQ(*(buf + i), 0xff);
  }

  Sylar::Schedule::Yield();
}

void MemoryCheckInvoke1() {
  const int sz = 128;
  char *buf = (char *)malloc(sz);
  memset(buf, 0xff, sz);

  std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
  attr->shared_mem_ = Sylar::SharedMem::AllocSharedMem(1, 128 * 1024);
  auto co1 =
      Sylar::Coroutine::CreateCoroutine(Sylar::Schedule::GetThreadSchedule(),
                                        attr, std::bind(&MemoryChecKInvoke2));
  auto co2 =
      Sylar::Coroutine::CreateCoroutine(Sylar::Schedule::GetThreadSchedule(),
                                        attr, std::bind(&MemoryChecKInvoke2));
  co1->Resume();
  co2->Resume();

  for (int i = 0; i < sz; i++) {
    EXPECT_EQ(*(buf + i), 0xff);
  }

  Sylar::Schedule::Yield();
}

TEST(Coroutine, SharedMemoryResume) {
  {
    std::shared_ptr<Sylar::CoroutineAttr> attr(new Sylar::CoroutineAttr());
    attr->shared_mem_ = Sylar::SharedMem::AllocSharedMem(1, 128 * 1024);
    auto co1 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Invoke1));
    auto co2 = Sylar::Coroutine::CreateCoroutine(
        Sylar::Schedule::GetThreadSchedule(), attr, std::bind(&Invoke1));
    co1->Resume();
    co2->Resume();
    co1->Resume();
    co2->Resume();
    EXPECT_EQ(co1->GetCoState(), Sylar::Coroutine::CO_TERMINAL);
    EXPECT_EQ(co2->GetCoState(), Sylar::Coroutine::CO_TERMINAL);
  }
}
