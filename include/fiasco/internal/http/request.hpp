#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace fiasco::detail {

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

http_method string_to_method(const std::string& m);
const char* method_to_string(http_method m);

struct request {
    http_method method = http_method::unknown;
    std::string url;           // Full URL (e.g. "/users/42?sort=name")
    std::string path;          // Path only (e.g. "/users/42")
    std::string query_string;  // Query string (e.g. "sort=name")
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    // Named path params
    std::unordered_map<std::string, std::string> path_params;

    // Positional path param values in URL order
    std::vector<std::string> ordered_path_params;

    [[nodiscard]] std::string header(const std::string& key) const {
        auto it = headers.find(key);
        return (it != headers.end()) ? it->second : "";
    }

    [[nodiscard]] std::string content_type() const { return header("Content-Type"); }
};

}  // namespace fiasco::detail