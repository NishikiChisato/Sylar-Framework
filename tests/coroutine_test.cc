#include "../src/include/coroutine.h"
#include "../src/include/mutex.h"
#include "../src/include/thread.h"
#include <functional>
#include <gtest/gtest.h>
#include <memory>

Sylar::Mutex m;
int cnt = 0;
int all = 1000;

void co_thr1() {
  for (int i = 0; i < all; i++) {
    cnt++;
  }
  std::cout << "sub step 1 finish, yield to main" << std::endl;
  Sylar::Coroutine::YieldToHold();
  for (int i = 0; i < all; i++) {
    cnt++;
  }
  std::cout << "sub step 2 finish, yield to main" << std::endl;
  Sylar::Coroutine::YieldToHold();
}

void thr1() {
  Sylar::Coroutine::CreateMainCo();
  Sylar::Coroutine::ptr c1(new Sylar::Coroutine(std::bind(co_thr1), 4 * 1024));
  std::cout << "main first swap" << std::endl;
  c1->Resume();
  std::cout << "main accept control, swap again" << std::endl;
  c1->Resume();
  std::cout << "main finish, swap again(no effect)" << std::endl;
  c1->Resume();
  std::cout << "main ending" << std::endl;

  c1->Reset(std::bind(&co_thr1));
  std::cout << "[resume]main first swap" << std::endl;
  c1->Resume();
  std::cout << "[resume]main accept control, swap again" << std::endl;
  c1->Resume();
  std::cout << "[resume]main finish, swap again(no effect)" << std::endl;
  c1->Resume();
  std::cout << "[resume]main ending" << std::endl;
}

TEST(Coroutine, Basic) {
  thr1();
  EXPECT_EQ(cnt, 4 * all);
}
