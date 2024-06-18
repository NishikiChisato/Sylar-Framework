#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include "singleton.h"
#include <cstdarg>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace Sylar {

class LogLevel {
public:
  enum Level {
    UNKNOW = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
  };

  static std::string ToString(LogLevel::Level);
};

class LogEvent {
public:
  typedef std::shared_ptr<LogEvent> ptr;
  LogEvent(std::string, std::string, LogLevel::Level, uint32_t, uint32_t,
           uint32_t, uint32_t, uint64_t);
  std::string GetLoggerName() const { return logger_name_; }
  std::string GetFilename() const { return filename_; }
  LogLevel::Level GetLogLevel() const { return level_; }
  uint32_t GetLine() const { return line_; }
  uint32_t GetEplase() const { return eplase_; }
  uint32_t GetThreadId() const { return thread_id_; }
  uint32_t GetCoroutineId() const { return coroutine_id_; }
  uint64_t GetTimestamp() const { return timestamp_; }
  std::stringstream &GetStream() { return stream_; }
  std::string GetStreamContent() { return stream_.str(); }

  // used for macro to format output
  void Format(const char *fmt, ...);

private:
  std::string logger_name_; // the name of logger that outputs the current event
  std::string filename_;    // the name of file
  LogLevel::Level level_;
  uint32_t line_;         // the line of log event
  uint32_t eplase_;       // the interval from program start to now
  uint32_t thread_id_;    // the ID of threads(equilvant to PID)
  uint32_t coroutine_id_; // the ID of coroutine
  uint64_t timestamp_;    // timestamp of event
  std::stringstream stream_;
};

class LogFormatter {
public:
  typedef std::shared_ptr<LogFormatter> ptr;

  class FormatterItem {
  public:
    typedef std::shared_ptr<FormatterItem> ptr;
    virtual ~FormatterItem(){};
    virtual void Format(std::ostream &os, LogEvent::ptr event) = 0;
  };

  LogFormatter(std::string pattern);
  std::string Format(LogEvent::ptr event);

#ifdef DEBUGMODE
  std::vector<std::tuple<std::string, std::string, int>> character_parsed_;
  std::vector<FormatterItem::ptr> items_parsed_;
#endif

private:
  void Init();
  std::string pattern_;
  std::vector<FormatterItem::ptr> items_;
};

class LogAppender {
public:
  typedef std::shared_ptr<LogAppender> ptr;
  virtual ~LogAppender() {}

  /**
   * @brief this function should implemente by derived class
   * this function simply output log by default format
   */
  virtual void Log(LogEvent::ptr event) = 0;
  virtual std::string GetAppenderName() = 0;
  LogAppender()
      : formatter_(new LogFormatter(
            "%d{%Y-%m-%d %H:%M:%S}%T%u%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n")) {}
  void SetFormatter(LogFormatter::ptr format) { formatter_ = format; }

protected:
  LogFormatter::ptr formatter_;
};

class LogAppenderToStd : public LogAppender {
public:
  void Log(LogEvent::ptr event) override;
  std::string GetAppenderName() override { return "LogAppenderToStd"; }
};

class LogAppenderToFile : public LogAppender {
public:
  LogAppenderToFile(std::string filename);
  ~LogAppenderToFile() { ofs_.close(); }
  void Log(LogEvent::ptr event) override;
  std::string GetAppenderName() override { return "LogAppenderToFile"; }

private:
  std::string filename_;
  std::ofstream ofs_;
};

class Logger {
public:
  typedef std::shared_ptr<Logger> ptr;
  Logger(std::string name = "root");

  std::string GetName() const { return name_; };
  LogLevel::Level GetMaxLevel() const { return max_level_; }
  void SetMaxLevel(LogLevel::Level level) { max_level_ = level; }
  LogLevel::Level GetMaxLevel() { return max_level_; }
  std::string GetMaxLevelToString() { return LogLevel::ToString(max_level_); }
  void AddAppender(LogAppender::ptr appender);
  void DelAppender(LogAppender::ptr appender);
  void CheckAppenders();
  void Log(LogEvent::ptr event);

private:
  std::string name_;
  LogLevel::Level max_level_; // the max level that this logger can output
  std::list<LogAppender::ptr> appenders_;
};

class LogEventWrapper {
public:
  LogEventWrapper(Logger::ptr lg, LogEvent::ptr ev) : logger_(lg), event_(ev) {}
  ~LogEventWrapper() { logger_->Log(event_); }
  std::stringstream &GetStream() { return event_->GetStream(); }
  LogEvent::ptr GetEvent() { return event_; }

private:
  Logger::ptr logger_;
  LogEvent::ptr event_;
};

#define SYLAR_LOG(logger, level)                                               \
  if (logger->GetMaxLevel() >= level)                                          \
  Sylar::LogEventWrapper(logger,                                               \
                         Sylar::LogEvent::ptr(new Sylar::LogEvent(             \
                             logger->GetName(), __FILE__, level, __LINE__, 0,  \
                             syscall(SYS_gettid), 0, std::time(nullptr))))     \
      .GetStream()

#define SYLAR_DEBUG_LOG(logger) SYLAR_LOG(logger, Sylar::LogLevel::DEBUG)

#define SYLAR_INFO_LOG(logger) SYLAR_LOG(logger, Sylar::LogLevel::INFO)

#define SYLAR_WARN_LOG(logger) SYLAR_LOG(logger, Sylar::LogLevel::WARN)

#define SYLAR_ERROR_LOG(logger) SYLAR_LOG(logger, Sylar::LogLevel::ERROR)

#define SYLAR_FATAL_LOG(logger) SYLAR_LOG(logger, Sylar::LogLevel::FATAL)

#define SYLAR_FMT_LOG(logger, level, fmt, ...)                                 \
  if (logger->GetMaxLevel() >= level)                                          \
  Sylar::LogEventWrapper(logger,                                               \
                         Sylar::LogEvent::ptr(new Sylar::LogEvent(             \
                             logger->GetName(), __FILE__, level, __LINE__, 0,  \
                             syscall(SYS_gettid), 0, std::time(nullptr))))     \
      .GetEvent()                                                              \
      ->Format(fmt, ##__VA_ARGS__)

#define SYLAR_FMT_DEBUG_LOG(logger, fmt, ...)                                  \
  SYLAR_FMT_LOG(logger, Sylar::LogLevel::DEBUG, fmt, ##__VA_ARGS__)

#define SYLAR_FMT_INFO_LOG(logger, fmt, ...)                                   \
  SYLAR_FMT_LOG(logger, Sylar::LogLevel::INFO, fmt, ##__VA_ARGS__)

#define SYLAR_FMT_WARN_LOG(logger, fmt, ...)                                   \
  SYLAR_FMT_LOG(logger, Sylar::LogLevel::WARN, fmt, ##__VA_ARGS__)

#define SYLAR_FMT_ERROR_LOG(logger, fmt, ...)                                  \
  SYLAR_FMT_LOG(logger, Sylar::LogLevel::ERROR, fmt, ##__VA_ARGS__)

#define SYLAR_FMT_FATAL_LOG(logger, fmt, ...)                                  \
  SYLAR_FMT_LOG(logger, Sylar::LogLevel::FATAL, fmt, ##__VA_ARGS__)

#define SYLAR_LOG_ROOT Sylar::LoggerMgr::GetInstance()->GetRoot()

/**
 * maybe we only pass one parameter, in this scenario, if we write Format(fmt,
 * __VA_ARGS__), we cannot complie success. we must add ## before __VA_ARGS__,
 * it will help us to delete comma before __VA_ARGS__
 *
 */

class LoggerManager {
public:
  LoggerManager();
  Logger::ptr GetRoot() { return root_; }
  Logger::ptr GetLogger(const std::string &name);
  Logger::ptr NewLogger(const std::string &name);

private:
  Logger::ptr root_;
  std::map<std::string, Logger::ptr> loggers_;
  std::mutex mu_;
};

typedef Sylar::SingletonPtr<LoggerManager> LoggerMgr;

// FormatterItem class
class MessageFormatterItem : public LogFormatter::FormatterItem {
public:
  MessageFormatterItem(std::string message = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    os << event->GetStreamContent();
  }
};

class LoggernameFormatterItem : public LogFormatter::FormatterItem {
public:
  LoggernameFormatterItem(const std::string &format = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    os << event->GetLoggerName();
  }
};

class FilenameFormatterItem : public LogFormatter::FormatterItem {
public:
  FilenameFormatterItem(const std::string &format = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    os << event->GetFilename();
  }
};

class LevelFormatterItem : public LogFormatter::FormatterItem {
public:
  LevelFormatterItem(const std::string &format = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    os << LogLevel::ToString(event->GetLogLevel());
  }
};

class LineFormatterItem : public LogFormatter::FormatterItem {
public:
  LineFormatterItem(const std::string &format = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    os << event->GetLine();
  }
};

class EplaseFormatterItem : public LogFormatter::FormatterItem {
public:
  EplaseFormatterItem(const std::string &format = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    os << event->GetEplase();
  }
};

class ThreadIdFormatterItem : public LogFormatter::FormatterItem {
public:
  ThreadIdFormatterItem(const std::string &format = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    os << event->GetThreadId();
  }
};

class CoroutineIdFormatterItem : public LogFormatter::FormatterItem {
public:
  CoroutineIdFormatterItem(const std::string &format = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    os << event->GetCoroutineId();
  }
};

class TimestampFormatterItem : public LogFormatter::FormatterItem {
public:
  TimestampFormatterItem(std::string format = "%Y-%m-%d %H:%M:%S")
      : time_format_(format) {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    std::time_t t = event->GetTimestamp();
    std::tm *now = std::localtime(&t);
    char buf[128];
    strftime(buf, sizeof buf, time_format_.c_str(), now);
    os << buf;
  }

private:
  std::string time_format_;
};

class TabFormatterItem : public LogFormatter::FormatterItem {
public:
  TabFormatterItem(const std::string &format = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override { os << "\t"; }
};

// line breaks
class BreaksFormatterItem : public LogFormatter::FormatterItem {
public:
  BreaksFormatterItem(const std::string &format = "") {}
  void Format(std::ostream &os, LogEvent::ptr event) override { os << "\n"; }
};

class StringFormatterItem : public LogFormatter::FormatterItem {
public:
  StringFormatterItem(const std::string &str = "") : content_(str) {}
  void Format(std::ostream &os, LogEvent::ptr event) override {
    os << content_;
  }

private:
  std::string content_;
};

} // namespace Sylar

#endif
