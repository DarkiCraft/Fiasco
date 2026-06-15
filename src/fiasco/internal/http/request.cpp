#include "fiasco/internal/http/request.hpp"

namespace fiasco::detail {

http_method string_to_method(const std::string& m) {
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

const char* method_to_string(http_method m) {
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

}  // namespace fiasco::detail