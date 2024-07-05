#include "../src/include/util.h"
#include <gtest/gtest.h>

TEST(Util, Backtrace) {
  std::cout << Sylar::BacktraceToString(100, 2) << std::endl;
  std::cout << Sylar::GetElapseFromRebootMS() << std::endl;
}