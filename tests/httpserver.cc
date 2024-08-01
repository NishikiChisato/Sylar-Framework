#include "../src/include/hook.hh"
#include "../src/include/http_server.hh"
#include <fstream>
#include <iostream>
#include <sstream>

auto index_handler =
    [](const Sylar::http::HttpRequest &request) -> Sylar::http::HttpResponse {
  auto path = "./file/index.html";
  std::fstream ifs(path);
  Sylar::http::HttpResponse res;
  if (!ifs.is_open()) {
    res = Sylar::http::HttpResponse::Create404Response();
    return res;
  }
  std::stringstream ss;
  ss << ifs.rdbuf();
  res.SetContent(ss.str());
  return res;
};

auto file1_handler =
    [](const Sylar::http::HttpRequest &request) -> Sylar::http::HttpResponse {
  auto path = "./file/file1.html";
  std::fstream ifs(path);
  Sylar::http::HttpResponse res;
  if (!ifs.is_open()) {
    res = Sylar::http::HttpResponse::Create404Response();
    return res;
  }
  std::stringstream ss;
  ss << ifs.rdbuf();
  res.SetContent(ss.str());
  return res;
};

auto file2_handler =
    [](const Sylar::http::HttpRequest &request) -> Sylar::http::HttpResponse {
  auto path = "./file/file2.html";
  std::fstream ifs(path);
  Sylar::http::HttpResponse res;
  if (!ifs.is_open()) {
    res = Sylar::http::HttpResponse::Create404Response();
    return res;
  }
  std::stringstream ss;
  ss << ifs.rdbuf();
  res.SetContent(ss.str());
  return res;
};

auto add_handler =
    [](const Sylar::http::HttpRequest &request) -> Sylar::http::HttpResponse {
  Sylar::http::HttpResponse res;
  auto path = request.GetPath();

  std::string::size_type prev_pos = 0, pos = 0;
  pos = path.find('?', prev_pos);
  prev_pos = pos + 1;

  if (pos == std::string::npos) {
    return Sylar::http::HttpResponse::Create404Response();
  }

  std::int64_t sum = 0;
  std::string param;
  std::stringstream ss(path.substr(prev_pos));

  while (std::getline(ss, param, '&')) {
    auto eq_pos = param.find('=');
    sum += std::stoi(param.substr(eq_pos + 1));
  }

  res.SetContent("<html><body>Sum: " + std::to_string(sum) + "</body></html>");
  return res;
};

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "error input, usage: " << std::string(argv[0])
              << " + ip address + port number" << std::endl;
    return 0;
  }
  Sylar::SetHookEnable(false);

  std::string host(argv[1]);
  std::string port(argv[2]);

  auto server =
      std::make_shared<Sylar::http::HttpServer>(host, std::stoi(port), true);
  server->Start();
  server->RegisterHttpRequestHandler("/", Sylar::http::HttpMethod::GET,
                                     index_handler);
  server->RegisterHttpRequestHandler("/file1", Sylar::http::HttpMethod::GET,
                                     file1_handler);
  server->RegisterHttpRequestHandler("/file2", Sylar::http::HttpMethod::GET,
                                     file2_handler);
  server->RegisterHttpRequestHandler("/add", Sylar::http::HttpMethod::GET,
                                     add_handler);

  std::cout << "server start..." << std::endl;
  std::cout << "type [quit] to quit server" << std::endl;
  std::string cmd;
  while (std::cin >> cmd, cmd != "quit") {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  server->Stop();
  std::cout << "server stop" << std::endl;

  return 0;
}