#include "include/config.h"
#include "include/log.h"
#include <boost/core/demangle.hpp>
#include <boost/lexical_cast.hpp>
#include <exception>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Sylar {

// Config
void Config::PrintAllVarsMap() {
  auto maps = GetVarsMap();
  for (auto it = maps.begin(); it != maps.end(); it++) {
    std::cout << "[" << it->first << ", " << it->second->GetDescription()
              << ", " << it->second->GetTypename() << "]" << std::endl;
  }
}

void Config::LoadFromYaml(const YAML::Node &root) {
  std::list<std::pair<std::string, YAML::Node>> list;
  YamlToList("", root, list);
  for (auto it = list.begin(); it != list.end(); it++) {
    auto key = it->first;
    if (key.empty()) {
      continue;
    }
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (GetVarsMap().find(key) != GetVarsMap().end()) {
      if (it->second.IsScalar()) {
        // current node is scalar, we directly get its value
        GetVarsMap()[it->first]->FromString(it->second.Scalar());
      } else {
        // YAML::Node overload operator<<, we use this to get value of current
        // node
        std::stringstream ss;
        ss << it->second;
        GetVarsMap()[key]->FromString(ss.str());
      }
    }
  }
}

void Config::YamlToList(const std::string &prefix, const YAML::Node &node,
                        std::list<std::pair<std::string, YAML::Node>> &list) {
  if (prefix.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstu"
                               "vwxyz._-123456789/") != std::string::npos) {
    SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
        << "Config::YamlToList accept an invalid argument: " << prefix;
    return;
  }

  list.push_back(std::make_pair(prefix, node));

  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); it++) {
      YamlToList(prefix.empty() ? it->first.Scalar()
                                : prefix + "." + it->first.Scalar(),
                 it->second, list);
    }
  }
}

class LogConfInit {
public:
  LogConfInit() {
    g_log_set->AddListener(1, [&](const std::set<LoggerConf> &old_val,
                                  const std::set<LoggerConf> &new_val) {
      for (auto &i : old_val) {
        if (new_val.find(i) == new_val.end()) {
          LoggerMgr::GetInstance()->DelLogger(i.name_);
          SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "log delete: " << i.name_;
        }
      }
      for (auto &i : new_val) {
        if (old_val.find(i) == old_val.end()) {
          SYLAR_INFO_LOG(SYLAR_LOG_ROOT) << "log add: " << i.name_;

          LoggerMgr::GetInstance()->NewLogger(i.name_);
          auto lg = LoggerMgr::GetInstance()->GetLogger(i.name_);
          lg->SetMaxLevel(LogLevel::FromString(i.level_));
          for (auto &j : i.appenders_) {
            std::string formatter;
            if (!j.formatter_.empty()) {
              formatter = j.formatter_;
            }
            if (!formatter.empty()) {
              if (j.type_ == "logappendertostd") {
                LogAppenderToStd::ptr apd(new LogAppenderToStd());
                LogFormatter::ptr fmt(new LogFormatter(formatter));
                apd->SetFormatter(fmt);
                lg->AddAppender(apd);
              } else if (j.type_ == "logappendertofile" && !j.file_.empty()) {
                LogAppenderToFile::ptr apd(new LogAppenderToFile(j.file_));
                LogFormatter::ptr fmt(new LogFormatter(formatter));
                apd->SetFormatter(fmt);
                lg->AddAppender(apd);
              }
            }
          }
        }
      }
    });
  }
};

static LogConfInit __logger_init;

} // namespace Sylar