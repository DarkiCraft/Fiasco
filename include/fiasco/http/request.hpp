#ifndef FIASCO_HTTP_REQUEST_HPP
#define FIASCO_HTTP_REQUEST_HPP

/// @file request.hpp
/// @brief The fiasco::request data type representing a parsed HTTP request.

#include <string>
#include <unordered_map>
#include <vector>

namespace fiasco {
/// @brief HTTP method enum.
enum class http_method {
  get,
  post,
  put,
  del,  // "delete" is a C++ keyword
  patch,
  head,
  options,
  unknown
};

/// @brief Converts a method string (e.g. "GET") to http_method enum.
inline http_method string_to_method(const std::string& m) {
  if (m == "GET") return http_method::get;
  if (m == "POST") return http_method::post;
  if (m == "PUT") return http_method::put;
  if (m == "DELETE") return http_method::del;
  if (m == "PATCH") return http_method::patch;
  if (m == "HEAD") return http_method::head;
  if (m == "OPTIONS") return http_method::options;
  return http_method::unknown;
}

/// @brief Converts http_method enum to string.
inline const char* method_to_string(http_method m) {
  switch (m) {
    case http_method::get:
      return "GET";
    case http_method::post:
      return "POST";
    case http_method::put:
      return "PUT";
    case http_method::del:
      return "DELETE";
    case http_method::patch:
      return "PATCH";
    case http_method::head:
      return "HEAD";
    case http_method::options:
      return "OPTIONS";
    default:
      return "UNKNOWN";
  }
}

/// @brief Represents a parsed HTTP request.
///
/// Built by the parser (llhttp_parser) from raw bytes. Passed to
/// route handlers either directly or after the framework extracts
/// typed parameters from it.
struct request {
  http_method method = http_method::unknown;
  std::string url;           // Full URL (e.g. "/users/42?sort=name")
  std::string path;          // Path only (e.g. "/users/42")
  std::string query_string;  // Query string (e.g. "sort=name")
  std::unordered_map<std::string, std::string> headers;
  std::string body;

  // Named path params — keyed by {name} from the route pattern.
  std::unordered_map<std::string, std::string> path_params;

  // Positional path param values in URL order — used by function_traits
  // for left-to-right primitive argument dispatch.
  std::vector<std::string> ordered_path_params;

  /// @brief Returns the value of a header, or empty string if not found.
  [[nodiscard]] std::string header(const std::string& key) const {
    auto it = headers.find(key);
    return (it != headers.end()) ? it->second : "";
  }

  /// @brief Returns the Content-Type header value.
  [[nodiscard]] std::string content_type() const {
    return header("Content-Type");
  }
};
}  // namespace fiasco

#endif  // FIASCO_HTTP_REQUEST_HPP
