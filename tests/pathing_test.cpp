#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "fiasco/internal/routing/pathing.hpp"

using namespace fiasco::detail;

// ============================================================
// split_path
// ============================================================

TEST_CASE("split_path root", "[pathing]") {
    auto segs = split_path("/");
    REQUIRE(segs.size() == 1);
    REQUIRE(segs[0] == "");
}

TEST_CASE("split_path simple", "[pathing]") {
    auto segs = split_path("/users");
    REQUIRE(segs.size() == 1);
    REQUIRE(segs[0] == "users");
}

TEST_CASE("split_path multi-segment", "[pathing]") {
    auto segs = split_path("/users/42/posts");
    REQUIRE(segs.size() == 3);
    REQUIRE(segs[0] == "users");
    REQUIRE(segs[1] == "42");
    REQUIRE(segs[2] == "posts");
}

TEST_CASE("split_path trailing slash", "[pathing]") {
    auto segs = split_path("/users/42/");
    REQUIRE(segs.size() == 3);
    REQUIRE(segs[0] == "users");
    REQUIRE(segs[1] == "42");
    REQUIRE(segs[2] == "");
}

TEST_CASE("split_path empty string", "[pathing]") {
    auto segs = split_path("");
    REQUIRE(segs.size() == 1);
    REQUIRE(segs[0] == "");
}

TEST_CASE("split_path double slash", "[pathing]") {
    auto segs = split_path("//a");
    REQUIRE(segs.size() == 2);
    REQUIRE(segs[0] == "");
    REQUIRE(segs[1] == "a");
}

TEST_CASE("split_path no leading slash", "[pathing]") {
    auto segs = split_path("just/a/path");
    REQUIRE(segs.size() == 3);
    REQUIRE(segs[0] == "just");
    REQUIRE(segs[1] == "a");
    REQUIRE(segs[2] == "path");
}

// ============================================================
// is_param_segment
// ============================================================

TEST_CASE("is_param_segment true", "[pathing]") {
    REQUIRE(is_param_segment("{id}"));
    REQUIRE(is_param_segment("{name}"));
    REQUIRE(is_param_segment("{user_id}"));
}

TEST_CASE("is_param_segment false", "[pathing]") {
    REQUIRE_FALSE(is_param_segment("id"));
    REQUIRE_FALSE(is_param_segment("{}"));
    REQUIRE_FALSE(is_param_segment("{"));
    REQUIRE_FALSE(is_param_segment("}"));
    REQUIRE_FALSE(is_param_segment(""));
    REQUIRE_FALSE(is_param_segment("{a"));
    REQUIRE_FALSE(is_param_segment("a}"));
}

// ============================================================
// param_name
// ============================================================

TEST_CASE("param_name basic", "[pathing]") {
    REQUIRE(param_name("{id}") == "id");
    REQUIRE(param_name("{user_id}") == "user_id");
    REQUIRE(param_name("{x}") == "x");
}

// ============================================================
// join_prefix
// ============================================================

TEST_CASE("join_prefix no prefix", "[pathing]") {
    REQUIRE(join_prefix("", "/users") == "/users");
    REQUIRE(join_prefix("", "users") == "users");
    REQUIRE(join_prefix("", "") == "");
}

TEST_CASE("join_prefix both with slashes", "[pathing]") {
    REQUIRE(join_prefix("/api/", "/users") == "/api/users");
    REQUIRE(join_prefix("/api/", "/users/") == "/api/users/");
}

TEST_CASE("join_prefix prefix without trailing slash", "[pathing]") {
    REQUIRE(join_prefix("/api", "/users") == "/api/users");
    REQUIRE(join_prefix("/api", "users") == "/api/users");
}

TEST_CASE("join_prefix path without leading slash", "[pathing]") {
    REQUIRE(join_prefix("/api/", "users") == "/api/users");
}

TEST_CASE("join_prefix empty path", "[pathing]") {
    REQUIRE(join_prefix("/api", "") == "/api");
    REQUIRE(join_prefix("/api/", "") == "/api/");
}

TEST_CASE("join_prefix both empty", "[pathing]") {
    REQUIRE(join_prefix("", "").empty());
}
