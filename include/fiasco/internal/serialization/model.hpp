#pragma once

#include <nlohmann/json.hpp>

/// Generates non-intrusive to_json / from_json for a struct.
///
/// Usage:
///   struct user {
///       std::string name;
///       int age;
///   };
///   FIASCO_MODEL(user, name, age)
///
/// This expands to NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(user, name, age),
/// enabling automatic JSON serialization and deserialization.
#define FIASCO_MODEL(Type, ...) NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Type, __VA_ARGS__)