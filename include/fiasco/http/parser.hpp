#ifndef FIASCO_HTTP_PARSER_HPP
#define FIASCO_HTTP_PARSER_HPP

/// @file parser.hpp
/// @brief llhttp-based HTTP parser producing fiasco::request objects.

#include <llhttp.h>

#include <string>

#include "request.hpp"

namespace fiasco::detail {
/// @brief Parser state accumulated during llhttp callbacks.
struct parser_state {
  request req;
  std::string current_header_field;
  std::string current_header_value;
  bool last_was_value = false;
  bool headers_complete = false;
  bool message_complete = false;
  std::string url_buf;
};

/// @brief Non-blocking HTTP/1.1 request parser backed by llhttp.
///
/// Feed raw bytes via feed(). When a complete request has been parsed,
/// is_complete() returns true, and you can retrieve the request via
/// take_request().
///
/// This is the default Parser template parameter for fiasco::server.
class llhttp_parser {
 public:
  llhttp_parser() {
    llhttp_settings_init(&m_settings);

    // -- Callbacks ---------------------------------------------------
    m_settings.on_url = [](llhttp_t* p, const char* at, size_t len) -> int {
      auto* s = static_cast<parser_state*>(p->data);
      s->url_buf.append(at, len);
      return 0;
    };

    m_settings.on_header_field = [](llhttp_t* p, const char* at,
                                    size_t len) -> int {
      auto* s = static_cast<parser_state*>(p->data);
      if (s->last_was_value) {
        // Flush the previous header
        if (!s->current_header_field.empty()) {
          s->req.headers[s->current_header_field] = s->current_header_value;
        }
        s->current_header_field.clear();
        s->current_header_value.clear();
        s->last_was_value = false;
      }
      s->current_header_field.append(at, len);
      return 0;
    };

    m_settings.on_header_value = [](llhttp_t* p, const char* at,
                                    size_t len) -> int {
      auto* s = static_cast<parser_state*>(p->data);
      s->current_header_value.append(at, len);
      s->last_was_value = true;
      return 0;
    };

    m_settings.on_headers_complete = [](llhttp_t* p) -> int {
      auto* s = static_cast<parser_state*>(p->data);
      // Flush last header
      if (!s->current_header_field.empty()) {
        s->req.headers[s->current_header_field] = s->current_header_value;
        s->current_header_field.clear();
        s->current_header_value.clear();
      }

      // Set method
      s->req.method = string_to_method(
          llhttp_method_name(static_cast<llhttp_method_t>(p->method)));

      // Parse URL into path + query_string
      s->req.url = s->url_buf;
      auto qpos = s->url_buf.find('?');
      if (qpos != std::string::npos) {
        s->req.path = s->url_buf.substr(0, qpos);
        s->req.query_string = s->url_buf.substr(qpos + 1);
      } else {
        s->req.path = s->url_buf;
      }

      s->headers_complete = true;
      return 0;
    };

    m_settings.on_body = [](llhttp_t* p, const char* at, size_t len) -> int {
      auto* s = static_cast<parser_state*>(p->data);
      s->req.body.append(at, len);
      return 0;
    };

    m_settings.on_message_complete = [](llhttp_t* p) -> int {
      auto* s = static_cast<parser_state*>(p->data);
      s->message_complete = true;
      return 0;
    };

    reset();
  }

  /// @brief Feeds raw bytes into the parser.
  /// @returns true if parsing succeeded (may need more data), false on error.
  bool feed(const char* data, size_t len) {
    auto err = llhttp_execute(&m_parser, data, len);
    if (err != HPE_OK && err != HPE_PAUSED) {
      m_error_reason = llhttp_errno_name(err);
      return false;
    }
    return true;
  }

  /// @brief Returns true when a complete HTTP request has been parsed.
  [[nodiscard]] bool is_complete() const noexcept {
    return m_state.message_complete;
  }

  /// @brief Moves the parsed request out and resets the parser for reuse.
  [[nodiscard]] request take_request() {
    request r = std::move(m_state.req);
    reset();
    return r;
  }

  /// @brief Returns the parse error reason (if feed() returned false).
  [[nodiscard]] const std::string& error() const noexcept {
    return m_error_reason;
  }

  /// @brief Resets the parser for a new request.
  void reset() {
    m_state = parser_state{};
    llhttp_init(&m_parser, HTTP_REQUEST, &m_settings);
    m_parser.data = &m_state;
    m_error_reason.clear();
  }

 private:
  llhttp_t m_parser{};
  llhttp_settings_t m_settings{};
  parser_state m_state;
  std::string m_error_reason;
};

}  // namespace fiasco::detail

#endif  // FIASCO_HTTP_PARSER_HPP
