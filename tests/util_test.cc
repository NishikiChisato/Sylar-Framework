#include "../src/include/util.hh"
#include <gtest/gtest.h>

TEST(Util, Backtrace) {
  std::cout << Sylar::BacktraceToString(100, 2) << std::endl;
  std::cout << Sylar::GetElapseFromRebootMS() << std::endl;
}