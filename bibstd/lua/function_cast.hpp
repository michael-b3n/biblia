#pragma once

#include "bibstd/lua/sol.hpp"
#include "bibstd/lua/value_cast.hpp"
#include "bibstd/util/exception.hpp"

#include <concepts>
#include <cstddef>
#include <expected>
#include <format>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace bibstd::lua
{
namespace detail
{

///
/// Result of a function that may fail: a script gets the value, or nil and the error, like io.open does it.
///
template<typename T>
struct is_expected_result final : std::false_type
{};
template<value_castable T, value_castable E>
struct is_expected_result<std::expected<T, E>> final : std::true_type
{};
template<typename T>
concept expected_result = is_expected_result<T>::value;

///
/// What a function for scripts returns: nothing, a value or an expected_result.
///
template<typename T>
concept function_result = std::is_void_v<T> || value_castable<T> || expected_result<T>;

///
/// \return the argument \p index of a call from a script, nil if it is left out
/// \throw util::exception if it is no T, the script gets it as error
///
template<value_castable T>
[[nodiscard]] auto argument(sol::state_view lua, const sol::variadic_args& arguments, const std::size_t index) -> T
{
  const auto object = index < arguments.size() ? arguments[static_cast<std::ptrdiff_t>(index)].get<sol::object>()
                                               : sol::make_object(lua, sol::lua_nil);
  auto value = value_cast<T>::from(object);
  if(!value)
  {
    throw util::exception{std::format("argument {} is of another type", index + 1)};
  }
  return std::move(*value);
}

///
/// What a function for scripts takes: a value value_cast converts, by value or const reference.
///
template<typename T>
concept function_parameter = value_castable<std::remove_cvref_t<T>> && std::convertible_to<std::remove_cvref_t<T>, T>;

///
/// \return the function a script calls in place of \p function, \see function_cast
///
template<typename R, typename... A>
  requires(function_result<R> && (function_parameter<A> && ...))
[[nodiscard]] auto wrapped(std::function<R(A...)> function)
{
  return [function = std::move(function)](sol::this_state lua, sol::variadic_args arguments)
  {
    const auto call = [&]<std::size_t... I>(std::index_sequence<I...>) -> R
    { return function(argument<std::remove_cvref_t<A>>(lua, arguments, I)...); };
    // Each kind of function_result
    if constexpr(std::is_void_v<R>)
    {
      call(std::index_sequence_for<A...>{});
    }
    else if constexpr(value_castable<R>)
    {
      return value_cast<R>::to(lua, call(std::index_sequence_for<A...>{}));
    }
    else if constexpr(expected_result<R>)
    {
      auto results = sol::variadic_results{};
      if(const auto result = call(std::index_sequence_for<A...>{}))
      {
        results.push_back(value_cast<typename R::value_type>::to(lua, *result));
      }
      else
      {
        results.push_back(sol::make_object(lua, sol::lua_nil));
        results.push_back(value_cast<typename R::error_type>::to(lua, result.error()));
      }
      return results;
    }
    else
    {
      static_assert(false, "function_result conversion unsupported");
    }
  };
}

///
/// \return \p function for as long as \p registered lives, called after that it throws
///
template<typename R, typename... A>
[[nodiscard]] auto guarded(std::weak_ptr<const void> registered, std::function<R(A...)> function) -> std::function<R(A...)>
{
  return [registered = std::move(registered), function = std::move(function)](A... arguments) -> R
  {
    if(registered.expired())
    {
      throw util::exception{"the function is not registered anymore"};
    }
    return function(std::forward<A>(arguments)...);
  };
}

} // namespace detail

///
/// Callables function_cast converts: one call signature, e.g. a lambda that is not generic,
/// the parameters and the result of types value_cast converts.
///
template<typename F>
concept function_castable = requires(F&& function) { detail::wrapped(std::function{std::forward<F>(function)}); };

///
/// The function a script calls for \p function, which takes and returns C++ values: value_cast converts them.
/// An argument of another type is an error for the script, one left out is nil, extra ones are ignored.
/// \return the function to set in a Lua table
///
template<function_castable F>
[[nodiscard]] auto function_cast(F&& function)
{
  return detail::wrapped(std::function{std::forward<F>(function)});
}

///
/// function_cast of a function taken back later: once \p registered is gone, calling it is an error for the script.
/// \return the function to set in a Lua table
///
template<function_castable F>
[[nodiscard]] auto function_cast(F&& function, std::weak_ptr<const void> registered)
{
  return detail::wrapped(detail::guarded(std::move(registered), std::function{std::forward<F>(function)}));
}

} // namespace bibstd::lua
