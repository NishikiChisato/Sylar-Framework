#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include "log.hh"
#include <boost/core/demangle.hpp>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Sylar {

// each config item has three parts: name, value, description
// the part of name and description can be abstracted to a base class
class ConfigVarBase {
public:
  typedef std::shared_ptr<ConfigVarBase> ptr;
  ConfigVarBase(const std::string &name = "", const std::string &dscp = "")
      : name_(name), description_(dscp) {}

  virtual ~ConfigVarBase() {}

  virtual bool FromString(const std::string &str) = 0;
  virtual std::string ToString() = 0;
  virtual std::string GetTypename() = 0;

  std::string GetName() const { return name_; }
  std::string GetDescription() const { return description_; }

private:
  std::string name_;
  std::string description_;
};

// cast from F to T
template <typename F, typename T> class LexicalCast {
public:
  T operator()(const F &in) { return boost::lexical_cast<T>(in); }
};

// cast from std::vector/std::list to std::string, vice versa

template <typename T> class LexicalCast<std::string, std::vector<T>> {
public:
  std::vector<T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::vector<T> v;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); it++) {
      ss.str("");
      ss << *it;
      v.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return v;
  }
};

template <typename T> class LexicalCast<std::vector<T>, std::string> {
public:
  std::string operator()(const std::vector<T> &v) {
    YAML::Node node;
    for (auto it = v.begin(); it != v.end(); it++) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(*it)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T> class LexicalCast<std::string, std::list<T>> {
public:
  std::list<T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::list<T> v;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); it++) {
      ss.str("");
      ss << *it;
      v.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return v;
  }
};

template <typename T> class LexicalCast<std::list<T>, std::string> {
public:
  std::string operator()(const std::list<T> &v) {
    YAML::Node node;
    for (auto it = v.begin(); it != v.end(); it++) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(*it)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// cast from std::set/std::unordered_set to std::string vice versa

template <typename T> class LexicalCast<std::string, std::set<T>> {
public:
  std::set<T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    std::set<T> v;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); it++) {
      ss.str("");
      ss << *it;
      v.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return v;
  }
};

template <typename T> class LexicalCast<std::set<T>, std::string> {
public:
  std::string operator()(const std::set<T> &v) {
    YAML::Node node;
    for (auto it = v.begin(); it != v.end(); it++) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(*it)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T> class LexicalCast<std::string, std::unordered_set<T>> {
public:
  std::unordered_set<T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::unordered_set<T> v;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); it++) {
      ss.str("");
      ss << *it;
      v.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return v;
  }
};

template <typename T> class LexicalCast<std::unordered_set<T>, std::string> {
public:
  std::string operator()(const std::unordered_set<T> &v) {
    YAML::Node node;
    for (auto it = v.begin(); it != v.end(); it++) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(*it)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// cast from std::map/std::unordered_map to std::string vice versa

template <typename T> class LexicalCast<std::string, std::map<std::string, T>> {
public:
  std::map<std::string, T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::map<std::string, T> m;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); it++) {
      ss.str("");
      // if it->second cannot be cast as std::string, it will occur error
      // we can use operator<<, which can definitely get std::string
      ss << it->second;
      m.insert(std::make_pair(it->first.Scalar(),
                              LexicalCast<std::string, T>()(ss.str())));
    }
    return m;
  }
};

template <typename T> class LexicalCast<std::map<std::string, T>, std::string> {
public:
  std::string operator()(const std::map<std::string, T> &v) {
    YAML::Node node;
    for (auto it = v.begin(); it != v.end(); it++) {
      node[it->first] = LexicalCast<T, std::string>()(it->second);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
  std::unordered_map<std::string, T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::unordered_map<std::string, T> m;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); it++) {
      ss.str("");
      // if it->second cannot be cast as std::string, it will occur error
      // we can use operator<<, which can definitely get std::string
      ss << it->second;
      m.insert(std::make_pair(it->first.Scalar(),
                              LexicalCast<std::string, T>()(ss.str())));
    }
    return m;
  }
};

template <typename T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
  std::string operator()(const std::unordered_map<std::string, T> &v) {
    YAML::Node node;
    for (auto it = v.begin(); it != v.end(); it++) {
      node[it->first] = LexicalCast<T, std::string>()(it->second);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <> class LexicalCast<std::string, LoggerConf> {
public:
  LoggerConf operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    LoggerConf lc;

    if (!node["name"].IsDefined()) {
      throw std::logic_error("log config logic error: name is not defined");
    }
    lc.name_ = node["name"].as<std::string>();

    if (!node["level"].IsDefined()) {
      throw std::logic_error("log config logic error: level is not defined");
    }
    lc.level_ = LogLevel::ToString(
        LogLevel::FromString(node["level"].as<std::string>()));

    if (!node["appenders"].IsDefined()) {
      throw std::logic_error(
          "log config logic error: appenders is not defined");
    } else if (!node["appenders"].IsSequence()) {
      throw std::logic_error(
          "log config logic error: appenders is not sequence");
    }

    for (int i = 0; i < node["appenders"].size(); i++) {
      LogAppenderConf lac;
      if (!node["appenders"][i]["type"].IsDefined()) {
        throw std::logic_error(
            "log config logic error: appenders's type is not defined");
      }
      lac.type_ = node["appenders"][i]["type"].as<std::string>();
      std::transform(lac.type_.begin(), lac.type_.end(), lac.type_.begin(),
                     ::tolower);

      if (node["appenders"][i]["formatter"].IsDefined()) {
        lac.formatter_ = node["appenders"][i]["formatter"].as<std::string>();
      }

      if (node["appenders"][i]["file"].IsDefined()) {
        lac.file_ = node["appenders"][i]["file"].as<std::string>();
      }

      lc.appenders_.push_back(lac);
    }
    return lc;
  }
};

template <> class LexicalCast<LoggerConf, std::string> {
public:
  std::string operator()(const LoggerConf &lc) {
    YAML::Node node;
    node["name"] = lc.name_;
    node["level"] = lc.level_;
    for (auto &i : lc.appenders_) {
      YAML::Node sub;
      if (!i.type_.empty()) {
        sub["type"] = i.type_;
      }
      if (!i.formatter_.empty()) {
        sub["formatter"] = i.formatter_;
      }
      if (!i.file_.empty()) {
        sub["file"] = i.file_;
      }
      node["appenders"].push_back(sub);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// since the type of value is different, this class should be declare to
// template class.
template <class T, class ToStr = LexicalCast<T, std::string>,
          class FromStr = LexicalCast<std::string, T>>
class ConfigVar : public ConfigVarBase {
public:
  typedef std::shared_ptr<ConfigVar> ptr;
  typedef std::function<void(const T &old_val, const T &new_val)>
      changeCallback;
  typedef std::map<uint64_t, changeCallback> callbackMap;

  ConfigVar(const std::string &name = "", const T &val = T(),
            const std::string &dscp = "")
      : ConfigVarBase(name, dscp), value_(val) {}

  bool FromString(const std::string &str) override {
    try {
      // value_ = FromStr()(str);
      SetValue(FromStr()(str));
      return true;
    } catch (std::exception &e) {
      SYLAR_WARN_LOG(SYLAR_LOG_ROOT)
          << "boost::lexical_cast from std::string to "
          << boost::core::demangle(typeid(T).name()) << " error: " << e.what();
      throw std::invalid_argument(str);
    }
    return false;
  }
  std::string ToString() override {
    try {
      std::string str = ToStr()(value_);
      return str;
    } catch (std::exception &e) {
      SYLAR_WARN_LOG(SYLAR_LOG_ROOT) << "boost::lexical_cast from "
                                     << boost::core::demangle(typeid(T).name())
                                     << " to std::string error: " << e.what();
    }
    return "";
  }

  std::string GetTypename() override {
    return boost::core::demangle(typeid(T).name());
  }

  T GetValue() const { return value_; }

  void SetValue(const T &val) {
    if (val == value_) {
      return;
    }
    // iterator callback function, pass old value and new value
    for (auto &item : callback_func_map_) {
      item.second(value_, val);
    }
    value_ = val;
  }

  void AddListener(uint64_t key, changeCallback func) {
    callback_func_map_[key] = func;
  }

  void DelListener(uint64_t key) { callback_func_map_.erase(key); }

  changeCallback GetListener(uint64_t key) { return callback_func_map_[key]; }

  void ClearListener() { callback_func_map_.clear(); }

private:
  T value_;
  callbackMap callback_func_map_;
};

// used to manage config variables
class Config {
public:
  typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

  /**
   * @brief lookup a config variable, return nullptr if current variable is not
   * exist
   * @return ConfigVar<T>::ptr derived class pointer
   */
  template <typename T>
  static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
    std::string key(name);
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    auto it = GetVarsMap().find(key);
    return it == GetVarsMap().end()
               ? nullptr
               : std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
  }

  /**
   * @brief lookup a config variable, return nullptr if current variable is not
   * exist
   * @return ConfigVar<T>::ptr derived class pointer
   */

  template <typename T>
  static typename ConfigVar<T>::ptr
  Lookup(const std::string &name, const T &val,
         const std::string &description = "") {
    std::string key(name);
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (auto ptr = GetVarsMap().find(key); ptr != GetVarsMap().end()) {
      auto ret = std::dynamic_pointer_cast<ConfigVar<T>>(ptr->second);
      if (ret) {
        return ret;
      }
      SYLAR_ERROR_LOG(SYLAR_LOG_ROOT) << "variable exist but type conflict";
      return nullptr;
    }
    if (key.find_first_not_of("abcdefghijklmnopqrstuvwxyz._-123456789/") !=
        std::string::npos) {

      std::string err =
          std::string("input argument ") + key + std::string(" is invalid");
      throw std::invalid_argument(err);
    }
    typename ConfigVar<T>::ptr ptr(new ConfigVar<T>(key, val, description));
    std::string str = ptr->ToString();
    GetVarsMap()[key] = ptr;
    return ptr;
  }

  /**
   * @brief if certain variable not exist in conf_vars, this function will
   * ignore this variable
   */
  static void LoadFromYaml(const YAML::Node &root);

  static void PrintAllVarsMap();

  static void
  DebugYamlToList(const std::string &prefix, const YAML::Node &node,
                  std::list<std::pair<std::string, YAML::Node>> &list) {
    YamlToList(prefix, node, list);
  }

private:
  static void YamlToList(const std::string &prefix, const YAML::Node &node,
                         std::list<std::pair<std::string, YAML::Node>> &list);
  static ConfigVarMap &GetVarsMap() {
    static ConfigVarMap conf_vars;
    return conf_vars;
  }
};

// config variable declaration

static ConfigVar<std::set<LoggerConf>>::ptr g_log_set = Config::Lookup(
    "logs", std::set<LoggerConf>(), "the set of pre-defined logs");

} // namespace Sylar

#endif