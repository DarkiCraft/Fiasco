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

inline http_method string_to_method(const std::string& m) {
    if (m == "GET") {
        return http_method::get;
    }
    if (m == "POST") {
        return http_method::post;
    }
    if (m == "PUT") {
        return http_method::put;
    }
    if (m == "DELETE") {
        return http_method::del;
    }
    if (m == "PATCH") {
        return http_method::patch;
    }
    if (m == "HEAD") {
        return http_method::head;
    }
    if (m == "OPTIONS") {
        return http_method::options;
    }
    return http_method::unknown;
}

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

    [[nodiscard]] std::string header(const std::string& key) const {
        auto it = headers.find(key);
        return (it != headers.end()) ? it->second : "";
    }

    [[nodiscard]] std::string content_type() const { return header("Content-Type"); }
};

}  // namespace fiasco::detail