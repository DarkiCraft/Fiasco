#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "fiasco/internal/http/request.hpp"
#include "fiasco/internal/routing/function_traits.hpp"

namespace fiasco::detail {

class router {
  public:
    router() = default;
    explicit router(std::string prefix)
        : m_prefix(std::move(prefix)) {}

    void print_routes() const {
        for (const auto& [method, map] : m_static_routes) {
            for (const auto& [path, _] : map) {
                std::cout << path << "\n";
            }
        }
        std::cout << "\n\n";
    }

    template <typename F>
    void get(const std::string& path, F&& f) {
        add(http_method::get, path, std::forward<F>(f));
    }

    template <typename F>
    void post(const std::string& path, F&& f) {
        add(http_method::post, path, std::forward<F>(f));
    }

    template <typename F>
    void put(const std::string& path, F&& f) {
        add(http_method::put, path, std::forward<F>(f));
    }

    template <typename F>
    void del(const std::string& path, F&& f) {
        add(http_method::del, path, std::forward<F>(f));
    }

    template <typename F>
    void patch(const std::string& path, F&& f) {
        add(http_method::patch, path, std::forward<F>(f));
    }

    void include_router(router sub, const std::string& prefix = "") {
        auto full_prefix = prefix + sub.m_prefix;
        for (auto& [method, path_map] : sub.m_static_routes) {
            for (auto& [path, handler] : path_map) {
                add_route(method, full_prefix + path, std::move(handler));
            }
        }
        for (auto& [method, entries] : sub.m_param_routes) {
            for (auto& entry : entries) {
                std::string pattern = full_prefix;
                for (const auto& seg : entry.segments) {
                    pattern += "/" + seg;
                }
                add_route(method, pattern, std::move(entry.handler));
            }
        }
    }

  private:
    /// "/users/42" -> ["users", "42"]
    inline std::vector<std::string> split_path(const std::string& path) const {
        std::vector<std::string> segs;
        std::string cur;
        cur.reserve(32);
        for (char c : path) {
            if (c == '/') {
                if (!cur.empty()) {
                    segs.push_back(std::move(cur));
                    cur.clear();
                }
            } else {
                cur += c;
            }
        }
        if (!cur.empty()) {
            segs.push_back(std::move(cur));
        }
        return segs;
    }

    inline bool is_param(const std::string& seg) {
        return seg.size() >= 3 && seg.front() == '{' && seg.back() == '}';
    }

    inline std::string param_name(const std::string& seg) { return seg.substr(1, seg.size() - 2); }

    struct match_result {
        bool matched = false;
        handler_fn handler;
        std::unordered_map<std::string, std::string> path_params;  // by name
        std::vector<std::string> ordered_path_params;              // by position
    };

    struct param_route_entry {
        std::vector<std::string> segments;  // pre-split pattern segments
        std::vector<std::string> names;     // param name per segment ("" if static)
        std::vector<bool> is_param_seg;     // pre-computed per-segment flag
        handler_fn handler;
    };

    const auto& static_routes() const { return m_static_routes; }
    const auto& param_routes() const { return m_param_routes; }

    void add_route(http_method method, const std::string& pattern, handler_fn handler) {
        auto segs = split_path(pattern);

        // Check if any segment is a param placeholder.
        bool has_param = false;
        for (const auto& s : segs) {
            if (is_param(s)) {
                has_param = true;
                break;
            }
        }

        if (!has_param) {
            // Static route: direct map insertion, O(1) match.
            m_static_routes[method][pattern] = std::move(handler);
        } else {
            // Parameterized route: precompute per-segment metadata once.
            param_route_entry entry;
            entry.segments = segs;
            entry.is_param_seg.reserve(segs.size());
            entry.names.reserve(segs.size());
            for (const auto& s : segs) {
                if (is_param(s)) {
                    entry.is_param_seg.push_back(true);
                    entry.names.push_back(param_name(s));
                } else {
                    entry.is_param_seg.push_back(false);
                    entry.names.emplace_back();
                }
            }
            entry.handler = std::move(handler);
            m_param_routes[method].push_back(std::move(entry));
        }
    }

    match_result match(http_method method, const std::string& path) const {
        // --- Static lookup ---
        {
            auto mit = m_static_routes.find(method);
            if (mit != m_static_routes.end()) {
                auto hit = mit->second.find(path);
                if (hit != mit->second.end()) {
                    return {true, hit->second, {}, {}};
                }
            }
        }

        // --- Parameterized scan ---
        auto pit = m_param_routes.find(method);
        if (pit == m_param_routes.end()) {
            return {false};
        }

        const auto req_segs = split_path(path);

        for (const auto& entry : pit->second) {
            if (entry.segments.size() != req_segs.size()) {
                continue;
            }

            std::unordered_map<std::string, std::string> params;
            std::vector<std::string> ordered;
            bool ok = true;

            for (size_t i = 0; i < entry.segments.size(); ++i) {
                if (entry.is_param_seg[i]) {
                    params[entry.names[i]] = req_segs[i];
                    ordered.push_back(req_segs[i]);
                } else if (entry.segments[i] != req_segs[i]) {
                    ok = false;
                    break;
                }
            }

            if (ok) {
                return {true, entry.handler, std::move(params), std::move(ordered)};
            }
        }

        return {false};
    }

    bool any_method_matches(const std::string& path) const {
        // Check static routes first across all methods.
        for (const auto& [method, map] : m_static_routes) {
            if (map.count(path)) {
                return true;
            }
        }

        const auto req_segs = split_path(path);
        for (const auto& [method, entries] : m_param_routes) {
            for (const auto& entry : entries) {
                if (entry.segments.size() != req_segs.size()) {
                    continue;
                }
                bool ok = true;
                for (size_t i = 0; i < entry.segments.size(); ++i) {
                    if (!entry.is_param_seg[i] && entry.segments[i] != req_segs[i]) {
                        ok = false;
                        break;
                    }
                }
                if (ok) {
                    return true;
                }
            }
        }
        return false;
    }

    template <typename F>
    void add(http_method method, const std::string& pattern, F&& f) {
        add_route(method, pattern, make_handler(std::forward<F>(f)));
    }

    const std::string m_prefix = "";

    std::unordered_map<http_method, std::unordered_map<std::string, handler_fn>> m_static_routes;

    std::unordered_map<http_method, std::vector<param_route_entry>> m_param_routes;

    friend class server;
};

}  // namespace fiasco::detail