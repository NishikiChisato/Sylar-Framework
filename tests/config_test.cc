#include "../src/include/config.h"
#include "../src/include/log.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <list>
#include <map>
#include <unistd.h>
#include <vector>

TEST(ConfigVar, Basic) {
  std::string name = "port";
  std::string description = "port number";
  int val = 10;
  Sylar::ConfigVar cfg(name, val, description);

  EXPECT_EQ(cfg.GetName(), name);
  EXPECT_EQ(cfg.GetValue(), val);
  EXPECT_EQ(cfg.GetDescription(), description);
}

TEST(Config, Basic) {
  std::string name1 = "port";
  std::string description1 = "port number";
  int val1 = 10;

  EXPECT_EQ(Sylar::Config::Lookup<int>(name1), nullptr);
  auto cfgVar1 = Sylar::Config::Lookup(name1, val1, description1);

  EXPECT_EQ(cfgVar1->GetName(), name1);
  EXPECT_EQ(cfgVar1->GetValue(), val1);
  EXPECT_EQ(cfgVar1->GetDescription(), description1);

  std::string name2 = "sys";
  std::string description2 = "system";
  std::string val2 = "unix";

  EXPECT_EQ(Sylar::Config::Lookup<std::string>(name2), nullptr);
  auto cfgVar2 = Sylar::Config::Lookup(name2, val2, description2);

  EXPECT_EQ(cfgVar2->GetName(), name2);
  EXPECT_EQ(cfgVar2->GetValue(), val2);
  EXPECT_EQ(cfgVar2->GetDescription(), description2);

  Sylar::Config::PrintAllVarsMap();
}

TEST(Config, Lookup) {
  std::string name1 = "&&**()()";
  std::string description1 = "something";
  int val1 = 10;

  try {
    Sylar::Config::Lookup(name1, val1, description1);
  } catch (std::exception &e) {
    SYLAR_DEBUG_LOG(SYLAR_LOG_ROOT) << e.what();
  }

  Sylar::Config::PrintAllVarsMap();

  std::string name2 = "port";
  std::cout << "this statement is normal" << std::endl;
  Sylar::Config::Lookup(name2, static_cast<int>(10));
  std::cout << "this statement is abnormal" << std::endl;
  Sylar::Config::Lookup(name2, static_cast<float>(10));
}

TEST(Config, DISABLED_YamlToList) {
  YAML::Node root = YAML::LoadFile("../tests/yaml/config_test.yaml");
  std::cout << YAML::Dump(root) << std::endl;
  std::cout << "=============================" << std::endl;
  std::list<std::pair<std::string, YAML::Node>> list;
  Sylar::Config::DebugYamlToList("", root, list);
  for (auto it = list.begin(); it != list.end(); it++) {
    // std::cout << "first = " << it->first << ", second = " << std::endl
    //           << YAML::Dump(it->second) << std::endl;
    std::cout << "first: " << std::endl << it->first << std::endl;
    std::cout << "second: " << std::endl
              << it->second << std::endl
              << std::endl;
  }
}

TEST(Config, BasicTypeCast) {
  Sylar::ConfigVar<int> intVar("int", 10);
  Sylar::ConfigVar<std::string> strVar("str", "std::string");
  Sylar::ConfigVar<double> douVar("double", 3.14);

  std::cout << intVar.ToString() << " " << strVar.ToString() << " "
            << douVar.ToString() << std::endl;

  intVar.FromString("20");
  strVar.FromString("string");
  douVar.FromString("2.71");

  std::cout << intVar.ToString() << " " << strVar.ToString() << " "
            << douVar.ToString() << std::endl;
}

template <typename T> void SequenceStringHelper(const T &v) {
  Sylar::ConfigVar<T> vecVar("vec", v);

  YAML::Node node;
  std::stringstream ss;
  for (auto &item : v) {
    node.push_back(item);
  }
  ss << node;
  EXPECT_EQ(ss.str(), vecVar.ToString());
  ss.str("");
  for (auto &item : v) {
    node.push_back(item);
  }
  ss << node;
  vecVar.FromString(ss.str());
  EXPECT_EQ(ss.str(), vecVar.ToString());
}

template <typename T> void SetStringHelper(const T &v) {
  Sylar::ConfigVar<T> vecVar("vec", v);

  YAML::Node node;
  std::stringstream ss;
  for (auto &item : v) {
    node.push_back(item);
  }
  ss << node;

  auto str = ss.str();
  auto astr = vecVar.ToString();

  std::sort(str.begin(), str.end());
  std::sort(astr.begin(), astr.end());
  EXPECT_EQ(str, astr);

  Sylar::ConfigVar<T> anotherVar;
  anotherVar.FromString(ss.str());

  str = ss.str();
  astr = vecVar.ToString();

  std::sort(str.begin(), str.end());
  std::sort(astr.begin(), astr.end());
  EXPECT_EQ(str, astr);
}

template <typename T> void MapStringHelper(const T &v) {
  Sylar::ConfigVar<T> vecVar("vec", v);

  YAML::Node node;
  std::stringstream ss;
  for (auto &item : v) {
    node[item.first] = item.second;
  }
  ss << node;

  auto str = ss.str();
  auto astr = vecVar.ToString();

  std::sort(str.begin(), str.end());
  std::sort(astr.begin(), astr.end());
  EXPECT_EQ(str, astr);

  Sylar::ConfigVar<T> anotherVar;
  anotherVar.FromString(ss.str());

  str = ss.str();
  astr = vecVar.ToString();

  std::sort(str.begin(), str.end());
  std::sort(astr.begin(), astr.end());
  EXPECT_EQ(str, astr);
}

TEST(Config, STLTypeCast) {
  SequenceStringHelper<std::vector<char>>({'1', '2', '3'});
  SequenceStringHelper<std::vector<short>>({1, 2, 3});
  SequenceStringHelper<std::vector<int>>({1, 2, 3});
  SequenceStringHelper<std::vector<long>>({1, 2, 3});
  SequenceStringHelper<std::vector<long long>>({1, 2, 3});
  SequenceStringHelper<std::vector<float>>({1.5, 2.5, 3.5});
  SequenceStringHelper<std::vector<double>>({1.5, 2.5, 3.5});
  SequenceStringHelper<std::vector<std::string>>({"123", "456", "789"});

  SequenceStringHelper<std::list<char>>({'1', '2', '3'});
  SequenceStringHelper<std::list<short>>({1, 2, 3});
  SequenceStringHelper<std::list<int>>({1, 2, 3});
  SequenceStringHelper<std::list<long>>({1, 2, 3});
  SequenceStringHelper<std::list<long long>>({1, 2, 3});
  SequenceStringHelper<std::list<float>>({1.5, 2.5, 3.5});
  SequenceStringHelper<std::list<double>>({1.5, 2.5, 3.5});
  std::list<std::string> l;
  l.push_back("123");
  l.push_back("456");
  l.push_back("789");
  SequenceStringHelper<std::list<std::string>>(l);

  SetStringHelper<std::set<char>>({'1', '2', '3'});
  SetStringHelper<std::set<short>>({1, 2, 3});
  SetStringHelper<std::set<int>>({1, 2, 3});
  SetStringHelper<std::set<long>>({1, 2, 3});
  SetStringHelper<std::set<long long>>({1, 2, 3});
  SetStringHelper<std::set<float>>({1.5, 2.5, 3.5});
  SetStringHelper<std::set<double>>({1.5, 2.5, 3.5});
  std::set<std::string> s;
  s.insert("123");
  s.insert("456");
  s.insert("789");
  SetStringHelper<std::set<std::string>>(s);

  SetStringHelper<std::unordered_set<char>>({'1', '2', '3'});
  SetStringHelper<std::unordered_set<short>>({1, 2, 3});
  SetStringHelper<std::unordered_set<int>>({1, 2, 3});
  SetStringHelper<std::unordered_set<long>>({1, 2, 3});
  SetStringHelper<std::unordered_set<long long>>({1, 2, 3});
  SetStringHelper<std::unordered_set<float>>({1.5, 2.5, 3.5});
  SetStringHelper<std::unordered_set<double>>({1.5, 2.5, 3.5});
  std::unordered_set<std::string> us;
  us.insert("123");
  us.insert("456");
  us.insert("789");
  SetStringHelper<std::unordered_set<std::string>>(us);

  MapStringHelper<std::map<std::string, char>>(
      {{"1", '1'}, {"2", '2'}, {"3", '3'}});
  MapStringHelper<std::map<std::string, short>>({{"1", 1}, {"2", 2}, {"3", 3}});
  MapStringHelper<std::map<std::string, int>>({{"1", 1}, {"2", 2}, {"3", 3}});
  MapStringHelper<std::map<std::string, long>>({{"1", 1}, {"2", 2}, {"3", 3}});
  MapStringHelper<std::map<std::string, long long>>(
      {{"1", 1}, {"2", 2}, {"3", 3}});
  MapStringHelper<std::map<std::string, float>>(
      {{"1", 1.5}, {"2", 2.5}, {"3", 3.5}});
  MapStringHelper<std::map<std::string, double>>(
      {{"1", 1.5}, {"2", 2.5}, {"3", 3.5}});
  std::map<std::string, std::string> m;
  m.insert({"1", "123"});
  m.insert({"2", "456"});
  m.insert({"3", "789"});
  MapStringHelper<std::map<std::string, std::string>>(m);

  MapStringHelper<std::unordered_map<std::string, char>>(
      {{"1", '1'}, {"2", '2'}, {"3", '3'}});
  MapStringHelper<std::unordered_map<std::string, short>>(
      {{"1", 1}, {"2", 2}, {"3", 3}});
  MapStringHelper<std::unordered_map<std::string, int>>(
      {{"1", 1}, {"2", 2}, {"3", 3}});
  MapStringHelper<std::unordered_map<std::string, long>>(
      {{"1", 1}, {"2", 2}, {"3", 3}});
  MapStringHelper<std::unordered_map<std::string, long long>>(
      {{"1", 1}, {"2", 2}, {"3", 3}});
  MapStringHelper<std::unordered_map<std::string, float>>(
      {{"1", 1.5}, {"2", 2.5}, {"3", 3.5}});
  MapStringHelper<std::unordered_map<std::string, double>>(
      {{"1", 1.5}, {"2", 2.5}, {"3", 3.5}});
  std::unordered_map<std::string, std::string> um;
  um.insert({"1", "123"});
  um.insert({"2", "456"});
  um.insert({"3", "789"});
  MapStringHelper<std::unordered_map<std::string, std::string>>(um);
}

class Person {
public:
  Person(const std::string &name = "", const std::string &sex = "",
         const int &age = 0)
      : name_(name), sex_(sex), age_(age) {}

  std::string ToString() {
    std::stringstream ss;
    ss << "[" << name_ << ", " << sex_ << ", " << age_ << "]" << std::endl;
    return ss.str();
  }

  std::string name_;
  std::string sex_;
  int age_;
};

bool operator==(const Person &lhs, const Person &rhs) {
  return lhs.name_ == rhs.name_ && lhs.sex_ == rhs.sex_ && lhs.age_ == rhs.age_;
}

// partical specialize
template <> class Sylar::LexicalCast<std::string, Person> {
public:
  Person operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    Person p;
    p.name_ = node["name"].as<std::string>();
    p.sex_ = node["sex"].as<std::string>();
    p.age_ = node["age"].as<int>();
    return p;
  }
};

template <> class Sylar::LexicalCast<Person, std::string> {
public:
  std::string operator()(const Person &p) {
    YAML::Node node;
    node["name"] = p.name_;
    node["sex"] = p.sex_;
    node["age"] = p.age_;
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

Sylar::ConfigVar<Person>::ptr g_person1 =
    Sylar::Config::Lookup("person1", Person(), "person 1");
Sylar::ConfigVar<Person>::ptr g_person2 =
    Sylar::Config::Lookup("person2", Person(), "person 2");
Sylar::ConfigVar<std::map<std::string, Person>>::ptr g_map1 =
    Sylar::Config::Lookup("map1", std::map<std::string, Person>(), "");
Sylar::ConfigVar<std::map<std::string, Person>>::ptr g_map2 =
    Sylar::Config::Lookup("map2", std::map<std::string, Person>(), "");
Sylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_map_vec =
    Sylar::Config::Lookup("map_vec",
                          std::map<std::string, std::vector<Person>>(), "");

TEST(Config, CustomType) {
  YAML::Node root = YAML::LoadFile("../tests/yaml/config_cuntom_type.yaml");
  Sylar::Config::LoadFromYaml(root);
  Sylar::ConfigVar<Person>::ptr p1 =
      Sylar::Config::Lookup("person1", Person(), "");
  Sylar::ConfigVar<Person>::ptr p2 =
      Sylar::Config::Lookup("person2", Person(), "");
  Sylar::ConfigVar<std::map<std::string, Person>>::ptr p3 =
      Sylar::Config::Lookup("map1", std::map<std::string, Person>(), "");
  Sylar::ConfigVar<std::map<std::string, Person>>::ptr p4 =
      Sylar::Config::Lookup("map2", std::map<std::string, Person>(), "");
  Sylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr p5 =
      Sylar::Config::Lookup("map_vec",
                            std::map<std::string, std::vector<Person>>(), "");

  std::cout << p1->ToString() << std::endl;
  std::cout << p2->ToString() << std::endl;
  std::cout << p3->ToString() << std::endl;
  std::cout << p4->ToString() << std::endl;
  std::cout << p5->ToString() << std::endl;
}

Sylar::ConfigVar<std::string>::ptr g_language =
    Sylar::Config::Lookup("language", std::string(), "");
Sylar::ConfigVar<std::string>::ptr g_regon =
    Sylar::Config::Lookup("regon", std::string(), "");
Sylar::ConfigVar<std::string>::ptr g_tcp_timeout_connection =
    Sylar::Config::Lookup("tcp.timeout.connection", std::string(), "");
Sylar::ConfigVar<std::string>::ptr g_tcp_timeout_request =
    Sylar::Config::Lookup("tcp.timeout.request", std::string(), "");
Sylar::ConfigVar<std::string>::ptr g_tcp_keep_alive =
    Sylar::Config::Lookup("tcp.keep-alive", std::string(), "");
Sylar::ConfigVar<std::string>::ptr g_tcp_max_connection =
    Sylar::Config::Lookup("tcp.max-connection", std::string(), "");

TEST(Config, CompositeType) {
  YAML::Node node = YAML::LoadFile("../tests/yaml/config_composite_type.yaml");
  Sylar::Config::LoadFromYaml(node);

  Sylar::ConfigVar<std::string>::ptr p1 =
      Sylar::Config::Lookup("language", std::string(), "");
  Sylar::ConfigVar<std::string>::ptr p2 =
      Sylar::Config::Lookup("regon", std::string(), "");
  Sylar::ConfigVar<std::string>::ptr p3 =
      Sylar::Config::Lookup("tcp.timeout.connection", std::string(), "");
  Sylar::ConfigVar<std::string>::ptr p4 =
      Sylar::Config::Lookup("tcp.timeout.request", std::string(), "");
  Sylar::ConfigVar<std::string>::ptr p5 =
      Sylar::Config::Lookup("tcp.keep-alive", std::string(), "");
  Sylar::ConfigVar<std::string>::ptr p6 =
      Sylar::Config::Lookup("tcp.max-connection", std::string(), "");

  std::cout << p1->GetName() << ": " << p1->ToString() << std::endl;
  std::cout << p2->GetName() << ": " << p2->ToString() << std::endl;
  std::cout << p3->GetName() << ": " << p3->ToString() << std::endl;
  std::cout << p4->GetName() << ": " << p4->ToString() << std::endl;
  std::cout << p5->GetName() << ": " << p5->ToString() << std::endl;
  std::cout << p6->GetName() << ": " << p6->ToString() << std::endl;
}

TEST(ConfigVariable, CallbackFuncBasic) {
  // change variable value
  g_language->SetValue("cn");
  g_language->AddListener(0, [](const std::string &l, const std::string &r) {
    SYLAR_INFO_LOG(SYLAR_LOG_ROOT)
        << "language value change from " << l << " to " << r;
  });
  YAML::Node root = YAML::LoadFile("../tests/yaml/config_composite_type.yaml");
  Sylar::Config::LoadFromYaml(root);

  g_language->SetValue("en");

  Sylar::ConfigVar<std::string>::ptr p =
      Sylar::Config::Lookup("language", std::string(), "");
  std::cout << p->GetName() << ": " << p->ToString() << std::endl;
}

TEST(Config, SupportLogModule) {
  YAML::Node node = YAML::LoadFile("../conf/log.yaml");
  Sylar::Config::LoadFromYaml(node);
  auto p1 = Sylar::Config::Lookup("logs", std::set<Sylar::LoggerConf>(), "");
  std::cout << p1->GetName() << ": " << p1->ToString() << std::endl;

  SYLAR_INFO_LOG(SYLAR_LOG_NAME("system")) << "system log message";

  std::filesystem::remove("./logs/root.log");
  std::filesystem::remove("./logs");

  Sylar::LoggerConf lf;
  lf.name_ = "another";
  lf.level_ = Sylar::LogLevel::ToString(Sylar::LogLevel::INFO);
  Sylar::LogAppenderConf lcf;
  lcf.type_ = "LogAppenderToStd";
  lf.appenders_.push_back(lcf);

  std::set<Sylar::LoggerConf> slf;
  slf.insert(lf);

  std::cout << Sylar::g_log_set->ToString() << std::endl;
  Sylar::g_log_set->SetValue(slf);
  std::cout << "log config changed" << std::endl;
  std::cout << Sylar::g_log_set->ToString() << std::endl;

  SYLAR_INFO_LOG(SYLAR_LOG_NAME("another")) << "another log message";
}