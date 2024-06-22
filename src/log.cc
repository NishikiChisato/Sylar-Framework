#include "include/log.h"
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Sylar {

// LogLevel
std::string LogLevel::ToString(LogLevel::Level level) {
  switch (level) {
#define XX(str)                                                                \
  case LogLevel::str:                                                          \
    return #str;

    XX(UNKNOW);
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
  default:
    return "<<LogLevel Error>>";
  }
}

LogLevel::Level LogLevel::FromString(const std::string &str) {
  std::string s = str;
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
#define XX(l, name)                                                            \
  if (#l == name) {                                                            \
    return LogLevel::l;                                                        \
  }

  XX(DEBUG, s);
  XX(INFO, s);
  XX(WARN, s);
  XX(ERROR, s);
  XX(FATAL, s);
#undef XX
  return LogLevel::UNKNOW;
}

// LogEvent
LogEvent::LogEvent(std::string name, std::string filename,
                   LogLevel::Level level, uint32_t line, uint32_t eplase,
                   uint32_t thread_id, uint32_t coroutine_id,
                   uint64_t timestamp)
    : logger_name_(name), filename_(filename), level_(level), line_(line),
      eplase_(eplase), thread_id_(thread_id), coroutine_id_(coroutine_id),
      timestamp_(timestamp) {}

void LogEvent::Format(const char *fmt, ...) {
  va_list val;
  va_start(val, fmt);
  char *buf;
  vasprintf(&buf, fmt, val);
  va_end(val);
  stream_ << buf;
  free(buf);
}

// LogAppdener
void LogAppenderToStd::Log(LogEvent::ptr event) {
  std::cout << formatter_->Format(event);
}

LogAppenderToFile::LogAppenderToFile(std::string filename)
    : filename_(filename) {
  ofs_.open(filename_, std::ios::out | std::ios::ate);
}

void LogAppenderToFile::Log(LogEvent::ptr event) {
  ofs_ << formatter_->Format(event);
}

// Logger
Logger::Logger(std::string name) : name_(name), max_level_(LogLevel::DEBUG) {}

void Logger::AddAppender(LogAppender::ptr item) { appenders_.push_back(item); }

void Logger::DelAppender(LogAppender::ptr item) {
  for (auto it = appenders_.begin(); it != appenders_.end(); ++it) {
    if (*it == item) {
      appenders_.erase(it);
    }
  }
}

void Logger::Log(LogEvent::ptr event) {
  for (auto &it : appenders_) {
    if (max_level_ >= event->GetLogLevel()) {
      it->Log(event);
    }
  }
}

void Logger::Flush(LogEvent::ptr event) {
  for (auto &it : appenders_) {
    if (max_level_ >= event->GetLogLevel()) {
      it->Flush();
    }
  }
}

void Logger::CheckAppenders() {
  for (auto &it : appenders_) {
    std::cout << it->GetAppenderName() << std::endl;
  }
}

// LogFormatter
LogFormatter::LogFormatter(std::string pattern) : pattern_(pattern) { Init(); }

void LogFormatter::Init() {
  // We check character in pattern_ one by one.
  // Each character has two categories, parsable character and unparsable.
  // Character in every tuple, the first store character, the third store flag
  // that judge this whether this character can be parsed(1 means parsable, 0
  // means unparsable), the second store details of this parsable character(e.g.
  // the content in curly brackets).
  //
  // clang-format off
  // pattern: "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n" 
  // charchter      details                   parsable&unparsable         meaning
  //    "d"           "%Y-%m-%d %H:%M:%S"             1                     current time
  //    "T"           ""                              1                     tab
  //    "t"	          ""						                  1                     thread id
  //    "T"           ""                              1                     tab
  //    "F"           ""                              1                     coroutine id
  //    "T"           ""                              1                     tab
  //    "["           ""                              0                     normal character
  //    "p"           ""                              1                     log level
  //    "]"           ""                              0                     normal character
  //    "T"           ""                              1                     tab
  //    "["           ""                              0                     normal character
  //    "c"           ""                              1                     the name of logger
  //    "]"           ""                              0                     normal character
  //    "T"           ""                              1                     tab
  //    "f"           ""                              1                     file name
  //    ":"           ""                              0                     normal character
  //    "l"           ""                              1                     line number 
  //    "T"           ""                              1                     tab
  //    "m"           ""                              1                     message
  //    "n"           ""                              1                     line breaks
  // clang-format on

  std::vector<std::tuple<std::string, std::string, int>> vec;
  std::string nstr; // store characters before %
  for (size_t i = 0; i < pattern_.size(); i++) {
    if (pattern_[i] != '%') {
      nstr.append(1, pattern_[i]);
      continue;
    }
    if (i + 1 < pattern_.size() && pattern_[i + 1] == '%') {
      nstr.append(1, '%');
      i++;
      continue;
    }
    size_t idx = i + 1;
    std::string character;
    std::string details;
    size_t start = 0;
    int flag = 0; // used to parse curly brackets
    while (idx < pattern_.size()) {
      // the content between the next chatacter of index i and the previous
      // character of nun-alpha character will be parsed to parasable character.
      if (!flag && !isalpha(pattern_[idx]) && pattern_[idx] != '{' &&
          pattern_[idx] != '}') {
        character = pattern_.substr(i + 1, idx - i - 1);
        break;
      }
      if (!flag && pattern_[idx] == '{') {
        character = pattern_.substr(i + 1, idx - i - 1);
        start = idx;
        flag = 1;
        idx++;
        continue;
      } else if (flag && pattern_[idx] == '}') {
        flag = 0;
        details = pattern_.substr(start + 1, idx - start - 1);
        idx++;
        break;
      }
      idx++;
      if (idx == pattern_.size()) {
        character = pattern_.substr(i + 1, idx - i - 1);
        break;
      }
    }

    if (!flag) {
      if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
        nstr.clear();
      }
      vec.push_back(std::make_tuple(character, details, 1));
      i = idx - 1;
    } else {
      if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
        nstr.clear();
      }
      vec.push_back(std::make_tuple("<<parse-error>>", "", 0));
    }
  }

#ifdef DEBUGMODE
  character_parsed_ = vec;
#endif

  std::map<std::string,
           std::function<LogFormatter::FormatterItem::ptr(std::string)>>
      m = {
#define XX(str, func)                                                            \
  {                                                                              \
#str,                                                                        \
        [&](const std::string &                                                  \
                fmt) { return LogFormatter::FormatterItem::ptr(new func(fmt)); } \
  }
          XX(m, MessageFormatterItem),    XX(p, LevelFormatterItem),
          XX(d, TimestampFormatterItem),  XX(T, TabFormatterItem),
          XX(t, ThreadIdFormatterItem),   XX(F, CoroutineIdFormatterItem),
          XX(c, LoggernameFormatterItem), XX(f, FilenameFormatterItem),
          XX(l, LineFormatterItem),       XX(n, BreaksFormatterItem),
          XX(u, EplaseFormatterItem)
#undef XX
      };

  for (auto &it : vec) {
    std::string str = std::get<0>(it);
    std::string details = std::get<1>(it);
    if (std::get<2>(it) == 1) {
      auto ptr = m.find(str);
      if (ptr != m.end()) {
        items_.push_back(ptr->second(details));
      } else {
        items_.push_back(LogFormatter::FormatterItem::ptr(
            new StringFormatterItem("<<parse-error>>")));
      }
    } else {
      items_.push_back(
          LogFormatter::FormatterItem::ptr(new StringFormatterItem(str)));
    }
  }

#ifdef DEBUGMODE
  items_parsed_ = items_;
#endif
}

std::string LogFormatter::Format(LogEvent::ptr event) {
  std::stringstream ss;
  for (auto &it : items_) {
    it->Format(ss, event);
  }
  return ss.str();
}

// LoggerManager

LoggerManager::LoggerManager() {
  GetRootLogger()->SetMaxLevel(LogLevel::FATAL);
  GetRootLogger()->AddAppender(LogAppenderToStd::ptr(new LogAppenderToStd()));
  loggers_[GetRootLogger()->GetName()] = GetRootLogger();
}

Logger::ptr LoggerManager::GetLogger(const std::string &name) {
  std::lock_guard<std::mutex> l(mu_);
  auto it = loggers_.find(name);
  if (it == loggers_.end()) {
    throw std::invalid_argument(name);
    return nullptr;
  } else {
    return it->second;
  }
}

Logger::ptr LoggerManager::NewLogger(const std::string &name) {
  std::lock_guard<std::mutex> l(mu_);
  auto it = loggers_.find(name);
  if (it == loggers_.end()) {
    Logger::ptr lg(new Logger(name));
    lg->AddAppender(LogAppenderToStd::ptr(new LogAppenderToStd()));
    loggers_[lg->GetName()] = lg;
    return lg;
  } else {
    return it->second;
  }
}

void LoggerManager::DelLogger(const std::string &name) {
  std::lock_guard<std::mutex> l(mu_);
  if (loggers_.find(name) == loggers_.end()) {
    return;
  }
  loggers_.erase(name);
}

// the following part implement the connection between log module and config
// variable module

bool operator==(const LogAppenderConf &lhs, const LogAppenderConf &rhs) {
  return lhs.type_ == rhs.type_ && lhs.file_ == rhs.file_ &&
         lhs.formatter_ == rhs.formatter_;
}

bool operator==(const LoggerConf &lhs, const LoggerConf &rhs) {
  return lhs.name_ == rhs.name_ && lhs.level_ == rhs.level_ &&
         lhs.appenders_ == rhs.appenders_;
}

bool operator<(const LoggerConf &lhs, const LoggerConf &rhs) {
  return lhs.name_ < rhs.name_;
}

} // namespace Sylar
