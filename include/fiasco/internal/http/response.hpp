#pragma once

#include <string>
#include <unordered_map>

namespace fiasco::detail {

struct response {
    int status_code = 200;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    static response to_empty(int status = 204);
    static response to_text(const std::string& body, int status = 200);
    static response to_json(const std::string& json_body, int status = 200);
    static response to_error(const std::string& message, int status);

    [[nodiscard]] static const char* reason_phrase(int code);

    [[nodiscard]] std::string serialize() const;
};

}  // namespace fiasco::detail