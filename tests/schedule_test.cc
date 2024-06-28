#include "../src/include/coroutine.h"
#include "../src/include/mutex.h"
#include "../src/include/schedule.h"
#include "../src/include/thread.h"
#include "../src/include/util.h"
#include <gtest/gtest.h>
#include <thread>

int g_cnt = 0;
int all = 1000;

void tsk1() {
  for (int i = 0; i < all; i++) {
    g_cnt++;
  }
}

void thr1() {

  Sylar::Schedule sc;
  sc.Start();
  sc.ScheduleTask(&tsk1);
  sc.ScheduleTask(&tsk1);
  sc.Stop();
  EXPECT_EQ(g_cnt, 2 * all);
}

TEST(Schedule, Basic) {
  std::thread t1(thr1);
  t1.join();
}