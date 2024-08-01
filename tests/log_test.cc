#include "../src/include/log.hh"
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

TEST(LogLevel, LogLevelToString) {

  EXPECT_EQ(Sylar::LogLevel::ToString(Sylar::LogLevel::UNKNOW), "UNKNOW");
  EXPECT_EQ(Sylar::LogLevel::ToString(Sylar::LogLevel::DEBUG), "DEBUG");
  EXPECT_EQ(Sylar::LogLevel::ToString(Sylar::LogLevel::INFO), "INFO");
  EXPECT_EQ(Sylar::LogLevel::ToString(Sylar::LogLevel::WARN), "WARN");
  EXPECT_EQ(Sylar::LogLevel::ToString(Sylar::LogLevel::ERROR), "ERROR");
  EXPECT_EQ(Sylar::LogLevel::ToString(Sylar::LogLevel::FATAL), "FATAL");
}

TEST(LogLevel, LogLevelFromString) {
  EXPECT_EQ(Sylar::LogLevel::FromString("debug"), Sylar::LogLevel::DEBUG);
  EXPECT_EQ(Sylar::LogLevel::FromString("info"), Sylar::LogLevel::INFO);
  EXPECT_EQ(Sylar::LogLevel::FromString("warn"), Sylar::LogLevel::WARN);
  EXPECT_EQ(Sylar::LogLevel::FromString("error"), Sylar::LogLevel::ERROR);
  EXPECT_EQ(Sylar::LogLevel::FromString("fatal"), Sylar::LogLevel::FATAL);
}

TEST(LogAppender, LogFunction) {
  Sylar::LogAppenderToStd().Log(Sylar::LogEvent::ptr(
      new Sylar::LogEvent("root", __FILE__, Sylar::LogLevel::DEBUG, __LINE__, 0,
                          syscall(SYS_gettid), 0, std::time(nullptr))));
  std::string filename = "log.txt";
  Sylar::LogAppenderToFile(filename).Log(Sylar::LogEvent::ptr(
      new Sylar::LogEvent("root", __FILE__, Sylar::LogLevel::DEBUG, __LINE__, 0,
                          syscall(SYS_gettid), 0, std::time(nullptr))));

  std::cout << "file content: " << std::endl;
  std::ifstream ifs;
  ifs.open(filename);
  std::string buf;
  std::getline(ifs, buf);
  std::cout << buf << std::endl;
  // should output two line

  std::remove(filename.c_str());
}

TEST(Logger, Basic) {
  Sylar::Logger lg;
  Sylar::LogEvent::ptr event(new Sylar::LogEvent(
      lg.GetName(), __FILE__, Sylar::LogLevel::DEBUG, __LINE__, 0,
      syscall(SYS_gettid), 0, std::time(nullptr)));
  Sylar::LogFormatter lf(
      "%d{%Y-%m-%d %H:%M:%S}%T%u%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");
  lg.AddAppender(Sylar::LogAppenderToStd::ptr(new Sylar::LogAppenderToStd()));
  lg.AddAppender(Sylar::LogAppenderToStd::ptr(new Sylar::LogAppenderToStd()));
  lg.AddAppender(Sylar::LogAppenderToStd::ptr(new Sylar::LogAppenderToStd()));
  // should output three identical line
  lg.Log(event);
}

TEST(LogFormatter, Init) {
  Sylar::Logger lg;
  Sylar::LogEvent::ptr event(new Sylar::LogEvent(
      lg.GetName(), __FILE__, Sylar::LogLevel::DEBUG, __LINE__, 0,
      syscall(SYS_gettid), 0, std::time(nullptr)));
  Sylar::LogFormatter lf(
      "%d{%Y-%m-%d %H:%M:%S}%T%u%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m  %% %asd "
      "test%n");
  event->GetStream() << "this is a message";

  lg.AddAppender(Sylar::LogAppenderToStd::ptr(new Sylar::LogAppenderToStd()));
  lg.Log(event);
  std::cout << lf.Format(event) << std::endl;
}

TEST(LogWrapper, Basic) {
  Sylar::Logger::ptr lg(new Sylar::Logger());
  Sylar::LogEvent::ptr event(new Sylar::LogEvent(
      lg->GetName(), __FILE__, Sylar::LogLevel::DEBUG, __LINE__, 0,
      syscall(SYS_gettid), 0, std::time(nullptr)));
  Sylar::LogFormatter lf(
      "%d{%Y-%m-%d %H:%M:%S}%T%u%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");
  lg->AddAppender(Sylar::LogAppenderToStd::ptr(new Sylar::LogAppenderToStd()));

  // output
  std::cout << lg->GetMaxLevelToString() << std::endl;
  Sylar::LogEventWrapper(lg, event).GetStream() << "this is a message";

  Sylar::LogEvent::ptr new_event(new Sylar::LogEvent(
      lg->GetName(), __FILE__, Sylar::LogLevel::INFO, __LINE__, 0,
      syscall(SYS_gettid), 0, std::time(nullptr)));

  // don't output
  std::cout << lg->GetMaxLevelToString() << std::endl;
  Sylar::LogEventWrapper(lg, new_event).GetStream() << "this is a message";

  // output
  lg->SetMaxLevel(Sylar::LogLevel::WARN);
  std::cout << lg->GetMaxLevelToString() << std::endl;
  Sylar::LogEventWrapper(lg, new_event).GetStream() << "this is a message";

  // output
  lg->SetMaxLevel(Sylar::LogLevel::ERROR);
  std::cout << lg->GetMaxLevelToString() << std::endl;
  SYLAR_LOG(lg, Sylar::LogLevel::ERROR) << "this is an error message";
}

TEST(LogWrapper, StreamInput) {
  std::string filename = "_log.txt";

  {
    Sylar::Logger::ptr lg(new Sylar::Logger());
    lg->SetMaxLevel(Sylar::LogLevel::INFO);

    Sylar::LogAppenderToStd::ptr std_apd(new Sylar::LogAppenderToStd());
    lg->AddAppender(std_apd);
    Sylar::LogFormatter::ptr std_fmt(new Sylar::LogFormatter(
        "%d{%Y-%m-%d %H:%M:%S}%T%u%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    std_apd->SetFormatter(std_fmt);

    Sylar::LogAppenderToFile::ptr file_apd(
        new Sylar::LogAppenderToFile(filename));
    file_apd->SetFormatter(std_fmt);
    lg->AddAppender(file_apd);

    SYLAR_DEBUG_LOG(lg) << "Debug";
    SYLAR_INFO_LOG(lg) << "Info";
    SYLAR_WARN_LOG(lg) << "Warn";
  }

  std::cout << "file content: " << std::endl;
  std::ifstream ifs;
  ifs.open(filename);
  std::string buf;
  while (std::getline(ifs, buf)) {
    std::cout << buf << std::endl;
  }

  std::remove(filename.c_str());
}

TEST(LogWrapper, FormatInput) {
  std::string filename = "_log.txt";

  {
    Sylar::Logger::ptr lg(new Sylar::Logger());
    lg->SetMaxLevel(Sylar::LogLevel::INFO);

    Sylar::LogAppenderToStd::ptr std_apd(new Sylar::LogAppenderToStd());
    lg->AddAppender(std_apd);
    Sylar::LogFormatter::ptr std_fmt(new Sylar::LogFormatter(
        "%d{%Y-%m-%d %H:%M:%S}%T%u%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    std_apd->SetFormatter(std_fmt);

    Sylar::LogAppenderToFile::ptr file_apd(
        new Sylar::LogAppenderToFile(filename));
    file_apd->SetFormatter(std_fmt);
    lg->AddAppender(file_apd);

    lg->CheckAppenders();

    SYLAR_FMT_DEBUG_LOG(lg, "Debug");
    SYLAR_FMT_DEBUG_LOG(lg, "Debug: number %d testing", 5);

    SYLAR_FMT_INFO_LOG(lg, "Info");
    SYLAR_FMT_DEBUG_LOG(lg, "Info: number %d testing", 5);

    SYLAR_FMT_WARN_LOG(lg, "Warn");
    SYLAR_FMT_DEBUG_LOG(lg, "Warn: number %d testing", 5);
  }

  std::cout << "file content: " << std::endl;
  std::ifstream ifs;
  ifs.open(filename);
  std::string buf;
  while (std::getline(ifs, buf)) {
    std::cout << buf << std::endl;
  }

  std::remove(filename.c_str());
}

TEST(LoggerManager, Basic) {
  auto mgr1 = Sylar::LoggerMgr::GetInstance();
  auto lg1 = mgr1->GetRoot();
  SYLAR_DEBUG_LOG(lg1);
  SYLAR_WARN_LOG(lg1);
  lg1->SetMaxLevel(Sylar::LogLevel::WARN);
  SYLAR_DEBUG_LOG(lg1);
  SYLAR_WARN_LOG(lg1);

  std::cout << "======================" << std::endl;

  auto mgr2 = Sylar::LoggerMgr::GetInstance();
  auto lg2 = mgr1->GetRoot();
  SYLAR_DEBUG_LOG(lg2);
  SYLAR_WARN_LOG(lg2);
  SYLAR_DEBUG_LOG(lg2);
  SYLAR_WARN_LOG(lg2);

  // singleton, we should set it to initial state
  lg1->SetMaxLevel(Sylar::LogLevel::DEBUG);
}

TEST(LogModule, TypicalUsage1) {
  auto mgr = Sylar::LoggerMgr::GetInstance();
  auto logger = mgr->GetRoot();
  std::cout << "Logger level is " << logger->GetMaxLevelToString() << std::endl;

  std::cout << "======================" << std::endl;

  std::cout << "Stream style output example" << std::endl;
  std::cout << "This message will output: " << std::endl;

  SYLAR_DEBUG_LOG(logger) << "this is a stream style";

  std::cout << "This message will not output: " << std::endl;

  SYLAR_INFO_LOG(logger) << "this is a stream style";

  std::cout << "======================" << std::endl;

  std::cout << "Format output output example" << std::endl;

  std::cout << "This message will not output: " << std::endl;

  SYLAR_FMT_DEBUG_LOG(logger, "this is a format style");

  std::cout << "This message will not output: " << std::endl;

  SYLAR_FMT_INFO_LOG(logger, "this is a format style\n");
}

TEST(LogModule, TypicalUsage2) {
  Sylar::Logger::ptr lg(new Sylar::Logger());
  Sylar::LogEvent::ptr event(new Sylar::LogEvent(
      lg->GetName(), __FILE__, Sylar::LogLevel::DEBUG, __LINE__, 0,
      syscall(SYS_gettid), 0, std::time(nullptr)));
  Sylar::LogAppender::ptr apd(new Sylar::LogAppenderToStd());
  Sylar::LogFormatter::ptr fmt(new Sylar::LogFormatter(
      "%d{%Y-%m-%d %H:%M:%S}%T[%c]%T[%p]%T%f:%l%T%m%n"));
  apd->SetFormatter(fmt);
  lg->AddAppender(apd);

  lg->Log(event);
  SYLAR_DEBUG_LOG(lg) << "this is stream style message";
  SYLAR_FMT_DEBUG_LOG(lg, "this is format style message");
}

TEST(LogModule, TypicalUsage3) {
  SYLAR_DEBUG_LOG(SYLAR_LOG_ROOT);
  SYLAR_DEBUG_LOG(SYLAR_LOG_ROOT) << "another message";
}
