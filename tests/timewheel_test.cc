#include "../src/include/timewheel.hh"
#include <gtest/gtest.h>

void Simple(int x) { std::cout << "function: " << x << std::endl; }

TEST(Timewheel, Basic) {
  auto tw = std::make_shared<Sylar::TimeWheel>(10, 1);

  tw->AddTimer(0, std::bind(&Simple, 0), nullptr, -1);
  for (int i = 1; i <= 20; i++) {
    tw->AddTimer(i, std::bind(&Simple, i), nullptr, 1);
  }

  std::cout << tw->Dump() << std::endl;

  std::cout << "first: [0, 1)" << std::endl;
  tw->ExecuteTimeout(1);
  std::cout << tw->Dump() << std::endl;

  std::cout << "second: [1, 15)" << std::endl;
  tw->ExecuteTimeout(15);
  std::cout << tw->Dump() << std::endl;
}
