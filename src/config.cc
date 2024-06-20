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

} // namespace Sylar