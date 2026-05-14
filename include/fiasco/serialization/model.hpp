#ifndef FIASCO_SERIALIZATION_MODEL_HPP
#define FIASCO_SERIALIZATION_MODEL_HPP

/// @file model.hpp
///
/// A thin wrapper around NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE that
/// generates to_json / from_json for a struct without modifying it.
/// Validation may be layered on top in the future.

#include <nlohmann/json.hpp>

/// @brief Generates non-intrusive to_json / from_json for a struct.
///
/// Usage:
/// @code
///   struct user {
///       std::string name;
///       int age;
///   };
///   FIASCO_MODEL(user, name, age)
/// @endcode
///
/// This expands to NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(user, name, age),
/// enabling automatic JSON serialization and deserialization.
#define FIASCO_MODEL(Type, ...) \
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Type, __VA_ARGS__)

#endif  // FIASCO_SERIALIZATION_MODEL_HPP
