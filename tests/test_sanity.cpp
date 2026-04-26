#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <string>

#include "fiasco/fiasco.hpp"

TEST_CASE("fiasco::version returns a non-empty string", "[sanity]") {
  const char* v = fiasco::version();
  REQUIRE(v != nullptr);
  REQUIRE(std::string(v) == "0.1.0");
}

TEST_CASE("nlohmann/json round-trip works", "[sanity]") {
  nlohmann::json j;
  j["name"] = "fiasco";
  j["version"] = 1;

  REQUIRE(j["name"] == "fiasco");
  REQUIRE(j["version"] == 1);

  // Serialize and deserialize
  std::string serialized = j.dump();
  auto parsed = nlohmann::json::parse(serialized);
  REQUIRE(parsed == j);
}

TEST_CASE("NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE works (future FIASCO_MODEL)",
          "[sanity]") {
  struct test_model {
    std::string username;
    int age;
  };

  // This is exactly what FIASCO_MODEL will expand to
  auto to_json = [](nlohmann::json& j, const test_model& m) {
    j = nlohmann::json{{"username", m.username}, {"age", m.age}};
  };

  auto from_json = [](const nlohmann::json& j, test_model& m) {
    j.at("username").get_to(m.username);
    j.at("age").get_to(m.age);
  };

  test_model original{"abdurrahman", 21};
  nlohmann::json j;
  to_json(j, original);

  test_model restored;
  from_json(j, restored);

  REQUIRE(restored.username == "abdurrahman");
  REQUIRE(restored.age == 21);
}
