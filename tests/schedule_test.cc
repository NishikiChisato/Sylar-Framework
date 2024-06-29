#include "../src/include/coroutine.h"
#include "../src/include/mutex.h"
#include "../src/include/schedule.h"
#include "../src/include/thread.h"
#include "../src/include/util.h"
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <vector>

int g_cnt1 = 0;
int g_cnt2 = 0;
int all = 1000;
Sylar::Mutex mu;

void tsk1() {
  for (int i = 0; i < all; i++) {
    Sylar::Mutex::ScopeLock l(mu);
    g_cnt1++;
  }
}

void tsk2() {
  for (int i = 0; i < all; i++) {
    Sylar::Mutex::ScopeLock l(mu);
    g_cnt2++;
  }
}

void thr1() {
  g_cnt1 = 0;
  Sylar::Schedule sc(2, true);
  sc.Start();
  sc.ScheduleTask(&tsk1);
  sc.ScheduleTask(&tsk1);
  sc.ScheduleTask(&tsk1);
  sc.ScheduleTask(&tsk1);
  sc.ScheduleTask(&tsk1);
  sc.Stop();
  EXPECT_EQ(g_cnt1, 5 * all);
}

void thr2() {
  g_cnt2 = 0;
  Sylar::Schedule sc(2, false);
  sc.Start();
  sc.ScheduleTask(&tsk2);
  sc.ScheduleTask(&tsk2);
  sc.ScheduleTask(&tsk2);
  sc.ScheduleTask(&tsk2);
  sc.ScheduleTask(&tsk2);
  sc.Stop();
  EXPECT_EQ(g_cnt2, 5 * all);
}

TEST(Schedule, BasicFunc) {
  Sylar::Thread t1(std::bind(&thr1), "t1"), t2(std::bind(&thr2), "t2");
  t1.Join();
  t2.Join();
}

void thr3() {
  g_cnt1 = 0;
  Sylar::Schedule sc(2, true);
  Sylar::Coroutine::ptr co1 =
      std::make_shared<Sylar::Coroutine>(std::bind(&tsk1));
  sc.Start();
  sc.ScheduleTask(co1);
  sc.ScheduleTask(co1);
  sc.ScheduleTask(co1);
  sc.ScheduleTask(co1);
  sc.ScheduleTask(co1);
  EXPECT_EQ(g_cnt1, 5 * all);
}

void thr4() {
  g_cnt2 = 0;
  Sylar::Schedule sc(2, false);
  Sylar::Coroutine::ptr co1 =
      std::make_shared<Sylar::Coroutine>(std::bind(&tsk1));
  co1->SetState(Sylar::Coroutine::READY);
  sc.Start();
  sc.ScheduleTask(co1);
  sc.ScheduleTask(co1);
  sc.ScheduleTask(co1);
  sc.ScheduleTask(co1);
  sc.ScheduleTask(co1);
  EXPECT_EQ(g_cnt2, 5 * all);
}

TEST(Schedule, BasicCoroutine) {
  Sylar::Thread t1(std::bind(&thr1), "t1"), t2(std::bind(&thr2), "t2");
  t1.Join();
  t2.Join();
}

void ManyTask(int threads, bool use_caller) {
  g_cnt1 = 0;
  g_cnt2 = 0;
  Sylar::Schedule sc(threads, use_caller);

  sc.Start();

  int tsk1_cnt = 0, tsk2_cnt = 0, all_task = 1000;

  std::random_device rd;
  std::mt19937 generator(rd());
  std::uniform_int_distribution<int> dis(0, 3);
  for (int i = 0; i < all_task; i++) {
    auto sel = dis(generator);
    if (sel == 0) {
      sc.ScheduleTask(&tsk1);
      tsk1_cnt++;
    } else if (sel == 1) {
      Sylar::Coroutine::ptr co1 =
          std::make_shared<Sylar::Coroutine>(std::bind(&tsk1));
      co1->SetState(Sylar::Coroutine::READY);
      sc.ScheduleTask(co1);
      tsk1_cnt++;
    } else if (sel == 2) {
      sc.ScheduleTask(&tsk2);
      tsk2_cnt++;
    } else {
      Sylar::Coroutine::ptr co2 =
          std::make_shared<Sylar::Coroutine>(std::bind(&tsk2));
      co2->SetState(Sylar::Coroutine::READY);
      sc.ScheduleTask(co2);
      tsk2_cnt++;
    }
  }

  sc.Stop();

  EXPECT_EQ(g_cnt1, tsk1_cnt * all);
  EXPECT_EQ(g_cnt2, tsk2_cnt * all);
}

TEST(Schedule, MangTasksWithSingleThreadUseCaller) { ManyTask(1, true); }

TEST(Schedule, MangTasksWithSingleThreadNoCaller) { ManyTask(1, false); }

TEST(Schedule, MangTasksWithMultipleThreadUseCaller) { ManyTask(10, true); }

TEST(Schedule, MangTasksWithMultipleThreadNoCaller) { ManyTask(10, false); }