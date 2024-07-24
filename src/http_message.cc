#include "include/http_message.hh"
#include <chrono>
#include <time.h>

namespace Sylar {

namespace http {

std::string MethodToString(HttpMethod method) {
  switch (method) {
#define XX(name)                                                               \
  case HttpMethod::name:                                                       \
    return #name;
    XX(GET)
    XX(HEAD)
    XX(POST)
    XX(PUT)
    XX(DELETE)
    XX(CONNECT)
    XX(OPTIONS)
    XX(TRACE)
    XX(PATCH)
#undef XX
  default:
    return std::string();
  }
}

std::string VersionToString(HttpVersion version) {
  switch (version) {
  case HttpVersion::HTTP_0_9:
    return "HTTP/0.9";
  case HttpVersion::HTTP_1_0:
    return "HTTP/1.0";
  case HttpVersion::HTTP_1_1:
    return "HTTP/1.1";
  case HttpVersion::HTTP_2_0:
    return "HTTP/2.0";
  default:
    return std::string();
  }
}

std::string StatusCodeToString(HttpStatusCode status) {
  switch (status) {
  case HttpStatusCode::Continue:
    return "Continue";
  case HttpStatusCode::Ok:
    return "OK";
  case HttpStatusCode::Accepted:
    return "Accepted";
  case HttpStatusCode::MovedPermanently:
    return "Moved Permanently";
  case HttpStatusCode::Found:
    return "Found";
  case HttpStatusCode::BadRequest:
    return "Bad Request";
  case HttpStatusCode::Forbidden:
    return "Forbidden";
  case HttpStatusCode::NotFound:
    return "Not Found";
  case HttpStatusCode::MethodNotAllowed:
    return "Method Not Allowed";
  case HttpStatusCode::ImATeapot:
    return "I'm a Teapot";
  case HttpStatusCode::InternalServerError:
    return "Internal Server Error";
  case HttpStatusCode::NotImplemented:
    return "Not Implemented";
  case HttpStatusCode::BadGateway:
    return "Bad Gateway";
  default:
    return std::string();
  }
}

HttpMethod MethodFromString(const std::string &method) {
  std::string upper_case;
  std::transform(method.begin(), method.end(), std::back_inserter(upper_case),
                 ::toupper);
#define XX(name)                                                               \
  else if (upper_case == #name) {                                              \
    return HttpMethod::name;                                                   \
  }
  if (upper_case == "GET") {
    return HttpMethod::GET;
  }
  XX(HEAD)
  XX(POST)
  XX(PUT)
  XX(DELETE)
  XX(CONNECT)
  XX(OPTIONS)
  XX(TRACE)
  XX(PATCH)
  else {
    throw std::invalid_argument("[MethodFromString] invalid method");
  }
#undef XX
}

HttpVersion VersionFromString(const std::string &version) {
  std::string upper;
  std::transform(version.begin(), version.end(), std::back_inserter(upper),
                 ::toupper);
  if (upper == "HTTP/0.9") {
    return HttpVersion::HTTP_0_9;
  } else if (upper == "HTTP/1.0") {
    return HttpVersion::HTTP_1_0;
  } else if (upper == "HTTP/1.1") {
    return HttpVersion::HTTP_1_1;
  } else if (upper == "HTTP/2.0") {
    return HttpVersion::HTTP_2_0;
  } else {
    throw std::invalid_argument("[VersionFromString] invalid version");
  }
}

std::string HttpRequest::ToString() {
  std::stringstream ss;
  ss << MethodToString(method_) << ' ' << GetPath() << ' '
     << VersionToString(version_) << "\r\n";
  for (const auto &it : headers_) {
    ss << it.first << ' ' << it.second << "\r\n";
  }
  ss << "\r\n";
  ss << content_;
  return ss.str();
}

std::string HttpResponse::ToString(bool send_content) {
  std::stringstream ss;
  ss << VersionToString(version_) << ' ' << static_cast<int>(status_code_)
     << ' ' << StatusCodeToString(status_code_) << "\r\n";
  for (const auto &it : headers_) {
    ss << it.first << ": " << it.second << "\r\n";
  }
  ss << "\r\n";
  if (send_content) {
    ss << content_ << "\r\n";
  }
  return ss.str();
}

void HttpRequest::FromString(const std::string &request) {
  std::stringstream ss;
  std::string start_line;
  std::string header_lines;
  std::string message_body;
  std::string line; // used to temporary store one line
  std::string method;
  std::string version;
  size_t prev_pos = 0, curr_pos = 0;
  // in request message, each line must appended by \r\n, so we can find this
  // string to find one line
  curr_pos = request.find("\r\n", prev_pos);
  if (curr_pos == std::string::npos) {
    throw std::invalid_argument(
        "[HttpRequest::FromString] cannot find start line in request message");
  }
  start_line = request.substr(0, curr_pos - prev_pos);
  prev_pos = curr_pos + 2;
  // the interval between the end of the last header line and message body is
  // \r\n\r\n. the first two is the end of last header line and the last two is
  // delimiter
  curr_pos = request.find("\r\n\r\n", prev_pos);
  if (curr_pos != std::string::npos) {
    // request message has header lines
    header_lines = request.substr(prev_pos, curr_pos - prev_pos);
    prev_pos = curr_pos + 4;
    if (prev_pos < request.length()) {
      message_body = request.substr(prev_pos);
    }
  }
  // now we have store start line, header lines and body message, we can extrace
  // value we need from these string
  ss.clear();
  ss.str(start_line);
  ss >> method >> path_ >> version;
  SetMethod(MethodFromString(method));
  SetVersion(VersionFromString(version));
  if (version_ != HttpVersion::HTTP_0_9 && version_ != HttpVersion::HTTP_1_0 &&
      version_ != HttpVersion::HTTP_1_1) {
    throw std::logic_error(
        "[HttpRequest::FromString] http version not support");
  }
  ss.clear();
  ss.str(header_lines);
  while (std::getline(ss, line)) {
    auto pos = line.find(':');
    std::string key = line.substr(0, pos);
    std::string val = line.substr(pos + 1);
    // remove all space in key and val
    key.erase(std::remove_if(key.begin(), key.end(),
                             [&](char c) { return std::isspace(c); }),
              key.end());
    val.erase(std::remove_if(val.begin(), val.end(),
                             [&](char c) { return std::isspace(c); }),
              val.end());
    SetHeaders(key, val);
  }
  SetContent(message_body);
}

std::string GetCurrentTime() {
  std::time_t now = std::time(nullptr);
  char buf[80];
  std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT",
                std::gmtime(&now));
  return buf;
}

HttpResponse HttpResponse::Create404Response() {
  HttpResponse res;
  res.SetHeaders("Data", GetCurrentTime());
  res.SetHeaders("Server", "CustomServer/1.0");
  res.SetHeaders("Content-Type", "text/html; charset=UTF-8");
  res.SetHeaders("Connection", "close");
  std::string content =
      "<html>\n"
      "<head><title>404 Not Found</title></head>\n"
      "<body>\n"
      "<h1>Not Found</h1>\n"
      "<p>The requested URL was not found on this server.</p>\n"
      "<hr>\n"
      "<address>CustomServer/1.0 Server</address>\n"
      "</body>\n"
      "</html>";
  res.SetContent(content);
  return res;
}

} // namespace http

} // namespace Sylar