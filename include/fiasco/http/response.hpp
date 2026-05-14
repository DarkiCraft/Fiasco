#ifndef FIASCO_HTTP_RESPONSE_HPP
#define FIASCO_HTTP_RESPONSE_HPP

/// @file response.hpp
/// @brief The fiasco::response data type representing an HTTP response.

#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unordered_map>

namespace fiasco {
using json = nlohmann::json;

/// @brief Represents an HTTP response to send to the client.
///
/// Can be constructed directly or via static factory methods.
/// The framework also auto-constructs responses when handlers return
/// FIASCO_MODEL types (serialized to JSON automatically).
struct response {
  int status_code = 200;
  std::unordered_map<std::string, std::string> headers;
  std::string body;

  // -- Static factory methods ------------------------------------------

  static response to_empty(int status = 204) {
    response r;
    r.status_code = status;
    return r;
  }

  /// @brief Creates a plain-text response.
  static response to_text(const std::string& body, int status = 200) {
    response r;
    r.status_code = status;
    r.body = body;
    r.headers["Content-Type"] = "text/plain";
    return r;
  }

  /// @brief Creates a JSON response from a pre-serialized JSON string.
  static response to_json(const std::string& json_body, int status = 200) {
    response r;
    r.status_code = status;
    r.body = json_body;
    r.headers["Content-Type"] = "application/json";
    return r;
  }

  /// @brief Creates an error response with a JSON body.
  ///
  /// Uses nlohmann/json for serialization so message strings containing
  /// quotes or backslashes produce valid JSON.
  static response to_error(const std::string& message, int status) {
    response r;
    r.status_code = status;
    nlohmann::json j;
    j["error"] = message;
    r.body = j.dump();
    r.headers["Content-Type"] = "application/json";
    return r;
  }

  // -- Serialization to raw HTTP ---------------------------------------

  /// @brief Returns the reason phrase for common status codes.
  [[nodiscard]] static const char* reason_phrase(int code) {
    switch (code) {
      case 200:
        return "OK";
      case 201:
        return "Created";
      case 204:
        return "No Content";
      case 400:
        return "Bad Request";
      case 404:
        return "Not Found";
      case 405:
        return "Method Not Allowed";
      case 422:
        return "Unprocessable Entity";
      case 500:
        return "Internal Server Error";
      default:
        return "Unknown";
    }
  }

  /// @brief Serializes this response into a raw HTTP/1.1 response string.
  [[nodiscard]] std::string serialize() const {
    std::ostringstream out;

    // Status line
    out << "HTTP/1.1 " << status_code << " " << reason_phrase(status_code)
        << "\r\n";

    // Headers
    bool has_content_length = false;
    bool has_connection = false;
    for (const auto& [key, val] : headers) {
      out << key << ": " << val << "\r\n";
      if (key == "Content-Length") has_content_length = true;
      if (key == "Connection") has_connection = true;
    }

    // Auto-add Content-Length if not set
    if (!has_content_length) {
      out << "Content-Length: " << body.size() << "\r\n";
    }

    // Auto-add Connection: close for now (keep-alive later)
    if (!has_connection) {
      out << "Connection: close\r\n";
    }

    // End of headers + body
    out << "\r\n" << body;

    return out.str();
  }
};

}  // namespace fiasco

#endif  // FIASCO_HTTP_RESPONSE_HPP
