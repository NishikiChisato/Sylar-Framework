#ifndef __SYLAR_HTTP_MESSAGE_HH__
#define __SYLAR_HTTP_MESSAGE_HH__

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

namespace Sylar {

namespace http {

enum class HttpMethod {
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE,
  PATCH
};

enum class HttpVersion { HTTP_0_9, HTTP_1_0, HTTP_1_1, HTTP_2_0 };

enum class HttpStatusCode {
  Continue = 100,
  SwitchingProtocols = 101,
  EarlyHints = 103,
  Ok = 200,
  Created = 201,
  Accepted = 202,
  NonAuthoritativeInformation = 203,
  NoContent = 204,
  ResetContent = 205,
  ParticalContent = 206,
  MultipleChoices = 300,
  MovedPermanently = 301,
  Found = 302,
  NotModified = 304,
  BadRequest = 400,
  UnAuthorized = 401,
  Forbidden = 403,
  NotFound = 404,
  MethodNotAllowed = 405,
  RequestTimeout = 408,
  ImATeapot = 418,
  InternalServerError = 500,
  NotImplemented = 501,
  BadGateway = 502,
  ServiceUnavailable = 503,
  GatewayTimeout = 504,
  HttpVersionNotSupported = 505
};

std::string MethodToString(HttpMethod method);
std::string VersionToString(HttpVersion version);
std::string StatusCodeToString(HttpStatusCode status);
HttpMethod MethodFromString(const std::string &method);
HttpVersion VersionFromString(const std::string &version);

class HttpMessageInterface {
public:
  HttpMessageInterface() : version_(HttpVersion::HTTP_1_0) {}
  virtual ~HttpMessageInterface() = default;

  HttpVersion GetVersion() const { return version_; }
  std::map<std::string, std::string> GetHeaders() const { return headers_; }
  std::string GetContent() const { return content_; }

  void SetVersion(HttpVersion version) { version_ = version; }
  void SetContent(const std::string &content) {
    content_ = content;
    SetContentLength();
  }
  void SetHeaders(const std::string &key, const std::string &val) {
    headers_[key] = val;
    SetContentLength();
  }

  void RemoveHeader(const std::string &key) { headers_.erase(key); }
  std::string GetHeader(const std::string &key) {
    if (headers_.count(key)) {
      return headers_[key];
    }
    return std::string();
  }

protected:
  void SetContentLength() {
    headers_["Content-Length"] = std::to_string(content_.length());
  }
  HttpVersion version_;
  std::map<std::string, std::string> headers_;
  std::string content_;
};

class HttpRequest : public HttpMessageInterface {
public:
  HttpRequest() : method_(HttpMethod::GET), HttpMessageInterface() {}
  ~HttpRequest() = default;

  HttpMethod GetMethod() const { return method_; }
  std::string GetPath() const { return path_; }

  void SetMethod(HttpMethod method) { method_ = method; }
  void SetPath(const std::string &path) { path_ = path; }

  std::string ToString();
  void FromString(const std::string &request);

private:
  void PathToLower() {
    std::transform(path_.begin(), path_.end(), path_.begin(), ::tolower);
  }

  HttpMethod method_;
  std::string path_;
};

class HttpResponse : public HttpMessageInterface {
public:
  HttpResponse() : status_code_(HttpStatusCode::Ok), HttpMessageInterface() {}
  HttpResponse(HttpStatusCode code)
      : status_code_(code), HttpMessageInterface() {}

  HttpStatusCode GetStatusCode() const { return status_code_; }

  void SetStatusCode(HttpStatusCode code) { status_code_ = code; }

  std::string ToString(bool send_content);

  static HttpResponse Create404Response();

private:
  HttpStatusCode status_code_;
};

} // namespace http

} // namespace Sylar

#endif