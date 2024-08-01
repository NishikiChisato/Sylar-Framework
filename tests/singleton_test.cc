#include "../src/include/singleton.hh"
#include <gtest/gtest.h>

class A {
public:
  int x = 0;
};
class B {
public:
  int y = 0;
};

TEST(SingletonTest, Basic) {
  // the second parameter is set to void by default
  auto is1 = Sylar::Singleton<A>::GetInstance();
  auto is2 = Sylar::Singleton<A>::GetInstance();
  // object itself are identical but its address are different
  EXPECT_EQ(is1, is2);
  // the second parameter is different, address are also different
  auto is3 = Sylar::Singleton<A, A>::GetInstance();
  EXPECT_NE(is1, is3);

  auto is4 = Sylar::Singleton<A, B>::GetInstance();
  EXPECT_NE(is3, is4);
  EXPECT_NE(is1, is4);
}

TEST(SingletonTest, BasicSharedPtr) {
  // the second parameter is set to void by default
  auto is1 = Sylar::SingletonPtr<A>::GetInstance();
  auto is2 = Sylar::SingletonPtr<A>::GetInstance();
  // object itself are identical but its address are different
  EXPECT_EQ(is1, is2);
  // the second parameter is different, address are also different
  auto is3 = Sylar::SingletonPtr<A, A>::GetInstance();
  EXPECT_NE(is1, is3);

  auto is4 = Sylar::SingletonPtr<A, B>::GetInstance();
  EXPECT_NE(is3, is4);
  EXPECT_NE(is1, is4);
}