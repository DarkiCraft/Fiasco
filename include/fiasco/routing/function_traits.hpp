#ifndef FIASCO_ROUTING_FUNCTION_TRAITS_HPP
#define FIASCO_ROUTING_FUNCTION_TRAITS_HPP

/// @file function_traits.hpp
/// @brief Compile-time handler introspection and auto-dispatch.
///
/// Dispatch rules (applied left-to-right per parameter):
///   1. fiasco::request        -> raw request passed through
///   2. Primitive (int, double, float, bool, std::string)
///      -> next positional path param, converted from string
///   3. Non-primitive with from_json (FIASCO_MODEL)
///      -> deserialized from JSON request body (exactly one allowed)
///   4. Anything else          -> resolved from DI container
///
/// Return value rules:
///   - fiasco::response        -> passed through as-is
///   - T with to_json          -> serialized to JSON via response::json()
///
/// Primitives CANNOT be the body. Wrap in a struct.

#include <any>
#include <concepts>
#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include "fiasco/http/request.hpp"
#include "fiasco/http/response.hpp"
#include "fiasco/routing/router.hpp"

namespace fiasco {

// -- DI container -------------------------------------------------------------

/// @brief Singleton-scoped DI container.
///
/// Factories are called once on first resolve; result is cached forever.
class di_container {
 public:
  /// @brief Registers a factory for type T. Called once; result is cached.
  template <typename T, std::invocable Factory>
  void provide(Factory&& factory) {
    auto key = std::type_index(typeid(T));
    m_factories[key] = [f = std::forward<Factory>(factory)]() -> std::any {
      return std::make_any<T>(f());
    };
  }

  /// @brief Resolves type T from the container. Returns a stable reference.
  /// @throws std::runtime_error if T was never registered.
  template <typename T>
  [[nodiscard]] T& resolve() {
    auto key = std::type_index(typeid(T));

    if (auto sit = m_singletons.find(key); sit != m_singletons.end()) {
      return std::any_cast<T&>(sit->second);
    }

    auto fit = m_factories.find(key);
    if (fit == m_factories.end()) {
      throw std::runtime_error(
          std::string("DI: no provider registered for type: ") +
          typeid(T).name());
    }

    m_singletons[key] = fit->second();
    return std::any_cast<T&>(m_singletons[key]);
  }

  /// @brief Returns true if T has been registered.
  template <typename T>
  [[nodiscard]] bool has() const {
    return m_factories.count(std::type_index(typeid(T))) > 0;
  }

 private:
  std::unordered_map<std::type_index, std::function<std::any()>> m_factories;
  std::unordered_map<std::type_index, std::any> m_singletons;
};

// -- Concepts -----------------------------------------------------------------

template <typename T>
concept Primitive = std::same_as<T, int> || std::same_as<T, long> ||
                    std::same_as<T, long long> || std::same_as<T, float> ||
                    std::same_as<T, double> || std::same_as<T, bool> ||
                    std::same_as<T, std::string>;

template <typename T>
concept FromJson = requires(const nlohmann::json& j, T& v) { from_json(j, v); };

template <typename T>
concept ToJson = requires(nlohmann::json& j, const T& v) { to_json(j, v); };

// -- Path param string -> T conversion ----------------------------------------

template <Primitive T>
[[nodiscard]] T convert_path_param(const std::string& s) {
  if constexpr (std::same_as<T, std::string>) {
    return s;
  } else if constexpr (std::same_as<T, int> || std::same_as<T, long>) {
    return static_cast<T>(std::stol(s));
  } else if constexpr (std::same_as<T, long long>) {
    return std::stoll(s);
  } else if constexpr (std::same_as<T, float>) {
    return std::stof(s);
  } else if constexpr (std::same_as<T, double>) {
    return std::stod(s);
  } else if constexpr (std::same_as<T, bool>) {
    return s == "true" || s == "1";
  }

  // unreachable
}

// -- Return value -> response serialization -----------------------------------

template <typename T>
[[nodiscard]] response serialize_return(T&& val) {
  using CleanT = std::decay_t<T>;
  if constexpr (std::same_as<CleanT, response>) {
    return std::forward<T>(val);
  } else if constexpr (ToJson<CleanT>) {
    nlohmann::json j = val;
    return response::to_json(j.dump());
  } else {
    static_assert(
        !sizeof(T),
        "Handler return type must be fiasco::response or a FIASCO_MODEL type");
  }

  return {};  // unreachable
}

// -- Lambda traits ------------------------------------------------------------

template <typename F>
struct lambda_traits : lambda_traits<decltype(&F::operator())> {};

template <typename C, typename R, typename... Args>
struct lambda_traits<R (C::*)(Args...) const> {
  using return_type = R;
  using args_tuple = std::tuple<Args...>;
  static constexpr size_t arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct lambda_traits<R (C::*)(Args...)> {
  using return_type = R;
  using args_tuple = std::tuple<Args...>;
  static constexpr size_t arity = sizeof...(Args);
};

// -- Per-argument resolver ----------------------------------------------------

/// @brief Resolves a single handler argument.
///
/// Dispatch order:
///   1. fiasco::request  -> pass raw request
///   2. Primitive        -> next positional path param
///   3. FromJson         -> deserialize request body
///   4. anything else    -> resolve from DI container
template <typename T>
decltype(auto) resolve_arg(const request& req, size_t& path_idx,
                           di_container& di) {
  using CleanT = std::decay_t<T>;

  if constexpr (std::same_as<CleanT, request>) {
    return req;
  } else if constexpr (Primitive<CleanT>) {
    if (path_idx >= req.ordered_path_params.size()) {
      throw std::runtime_error(
          "Not enough path params for handler argument at position " +
          std::to_string(path_idx));
    }
    return static_cast<CleanT>(
        convert_path_param<CleanT>(req.ordered_path_params[path_idx++]));
  } else if constexpr (FromJson<CleanT>) {
    if (req.body.empty()) {
      throw std::runtime_error(
          "Handler expects a JSON body but request body is empty");
    }
    try {
      auto j = nlohmann::json::parse(req.body);
      CleanT val;
      from_json(j, val);
      return val;
    } catch (const nlohmann::json::exception& e) {
      throw std::runtime_error(std::string("JSON body parse error: ") +
                               e.what());
    }
  } else {
    return static_cast<T>(di.resolve<CleanT>());
  }
}

// -- Dispatcher ---------------------------------------------------------------

template <typename F, typename ArgsTuple, size_t... Is>
response dispatch_impl(F& fn, const request& req, size_t& path_idx,
                       di_container& di, std::index_sequence<Is...>) {
  if constexpr (std::is_void_v<std::invoke_result_t<
                    F, std::tuple_element_t<Is, ArgsTuple>...>>) {
    fn(resolve_arg<std::tuple_element_t<Is, ArgsTuple>>(req, path_idx, di)...);
    return response::to_empty();
  } else {
    return serialize_return(fn(resolve_arg<std::tuple_element_t<Is, ArgsTuple>>(
        req, path_idx, di)...));
  }
}

// -- make_handler -------------------------------------------------------------

/// @brief Wraps any compatible callable into a normalized handler_fn.
///
/// The DI container is captured by reference — it must outlive all handlers.
template <typename F>
[[nodiscard]] handler_fn make_handler(F&& f, di_container& di) {
  using traits = lambda_traits<std::decay_t<F>>;
  using args_tuple = traits::args_tuple;
  constexpr size_t arity = traits::arity;

  return [fn = std::forward<F>(f), &di](request req) mutable -> response {
    size_t path_idx = 0;
    return dispatch_impl<decltype(fn), args_tuple>(
        fn, req, path_idx, di, std::make_index_sequence<arity>{});
  };
}

}  // namespace fiasco

#endif  // FIASCO_ROUTING_FUNCTION_TRAITS_HPP