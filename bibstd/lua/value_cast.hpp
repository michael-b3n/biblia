#pragma once

#include "bibstd/lua/sol.hpp"
#include "bibstd/meta/chrono.hpp"
#include "bibstd/util/numeric_cast.hpp"
#include "bibstd/util/ranges.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace bibstd::lua
{

///
/// Conversion between a C++ value and the Lua value a script sees: to(lua, value) -> sol::object and
/// from(object) -> std::optional<T>, std::nullopt if the object is no T. Both copy.
/// Only plain value types convert, no references, pointers, views or const types: they would point into a Lua
/// value that may be gone.
///
template<typename T>
struct value_cast final
{
  // Constants
  static constexpr auto valid = false;
};

///
/// Types value_cast converts.
///
template<typename T>
concept value_castable = value_cast<T>::valid;

namespace detail
{

///
/// A value type of its own, no reference and neither const nor volatile.
///
template<typename T>
concept plain = std::same_as<T, std::remove_cvref_t<T>>;

///
/// \return the values of \p items, std::nullopt if one of them is none
///
template<std::ranges::input_range R>
[[nodiscard]] auto all_or_none(R&& items) -> std::optional<std::vector<typename std::ranges::range_value_t<R>::value_type>>;

///
/// \return the keys and values of \p table
///
[[nodiscard]] auto entries(const sol::table& table) -> std::vector<std::pair<sol::object, sol::object>>;

} // namespace detail

///
/// A Lua boolean.
///
template<>
struct value_cast<bool> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, bool v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<bool>;
};

///
/// A Lua integer, a float is none even of an integral value. Out of the range of T it is none.
/// \throw boost::numeric::bad_numeric_cast from to, if Lua can not hold the value
///
template<std::integral T>
  requires(detail::plain<T> && !std::same_as<T, bool>)
struct value_cast<T> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, T v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<T>;
};

///
/// A Lua number.
///
template<std::floating_point T>
  requires detail::plain<T>
struct value_cast<T> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, T v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<T>;
};

///
/// A Lua string.
///
template<>
struct value_cast<std::string> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, const std::string& v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<std::string>;
};

///
/// A duration is its count.
///
template<typename T>
  requires(detail::plain<T> && meta::is_duration_v<T>)
struct value_cast<T> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, const T& v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<T>;
};

///
/// A file system path is a string with '/' as separator.
///
template<>
struct value_cast<std::filesystem::path> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, const std::filesystem::path& v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<std::filesystem::path>;
};

///
/// nil or a T.
///
template<value_castable T>
struct value_cast<std::optional<T>> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, const std::optional<T>& v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<std::optional<T>>;
};

///
/// A table of the values in sequence, one with other keys is none.
///
template<value_castable T>
struct value_cast<std::vector<T>> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, const std::vector<T>& v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<std::vector<T>>;
};

///
/// A table of the values by key. Lua takes any value but nil as key, so \p K is no std::optional.
///
template<value_castable K, value_castable T>
struct value_cast<std::map<K, T>> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, const std::map<K, T>& v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<std::map<K, T>>;
};

///
///
inline auto detail::entries(const sol::table& table) -> std::vector<std::pair<sol::object, sol::object>>
{
  auto result = std::vector<std::pair<sol::object, sol::object>>{};
  table.for_each([&](const sol::object& key, const sol::object& value) { result.emplace_back(key, value); });
  return result;
}

///
///
template<std::ranges::input_range R>
auto detail::all_or_none(R&& items) -> std::optional<std::vector<typename std::ranges::range_value_t<R>::value_type>>
{
  const auto values = std::forward<R>(items) | std::ranges::to<std::vector>();
  if(!std::ranges::all_of(values, [](const auto& value) { return value.has_value(); }))
  {
    return std::nullopt;
  }
  return values | std::views::transform([](const auto& value) { return *value; }) | std::ranges::to<std::vector>();
}

///
///
inline auto value_cast<bool>::to(sol::state_view lua, const bool v) -> sol::object
{
  return sol::make_object(lua, v);
}

///
///
inline auto value_cast<bool>::from(const sol::object& v) -> std::optional<bool>
{
  return v.get_type() == sol::type::boolean ? std::optional{v.as<bool>()} : std::nullopt;
}

///
///
template<std::integral T>
  requires(detail::plain<T> && !std::same_as<T, bool>)
auto value_cast<T>::to(sol::state_view lua, const T v) -> sol::object
{
  return sol::make_object(lua, numeric_cast<lua_Integer>(v));
}

///
///
template<std::integral T>
  requires(detail::plain<T> && !std::same_as<T, bool>)
auto value_cast<T>::from(const sol::object& v) -> std::optional<T>
{
  // With the safeties on, sol takes integral numbers only
  const auto value = v.as<sol::optional<lua_Integer>>();
  return value && std::in_range<T>(*value) ? std::optional{static_cast<T>(*value)} : std::nullopt;
}

///
///
template<std::floating_point T>
  requires detail::plain<T>
auto value_cast<T>::to(sol::state_view lua, const T v) -> sol::object
{
  return sol::make_object(lua, static_cast<lua_Number>(v));
}

///
///
template<std::floating_point T>
  requires detail::plain<T>
auto value_cast<T>::from(const sol::object& v) -> std::optional<T>
{
  return v.get_type() == sol::type::number ? std::optional{static_cast<T>(v.as<lua_Number>())} : std::nullopt;
}

///
///
inline auto value_cast<std::string>::to(sol::state_view lua, const std::string& v) -> sol::object
{
  return sol::make_object(lua, v);
}

///
///
inline auto value_cast<std::string>::from(const sol::object& v) -> std::optional<std::string>
{
  return v.get_type() == sol::type::string ? std::optional{v.as<std::string>()} : std::nullopt;
}

///
///
template<typename T>
  requires(detail::plain<T> && meta::is_duration_v<T>)
auto value_cast<T>::to(sol::state_view lua, const T& v) -> sol::object
{
  return value_cast<typename T::rep>::to(lua, v.count());
}

///
///
template<typename T>
  requires(detail::plain<T> && meta::is_duration_v<T>)
auto value_cast<T>::from(const sol::object& v) -> std::optional<T>
{
  return value_cast<typename T::rep>::from(v).transform([](const auto count) { return T{count}; });
}

///
///
inline auto value_cast<std::filesystem::path>::to(sol::state_view lua, const std::filesystem::path& v) -> sol::object
{
  return value_cast<std::string>::to(lua, v.generic_string());
}

///
///
inline auto value_cast<std::filesystem::path>::from(const sol::object& v) -> std::optional<std::filesystem::path>
{
  return value_cast<std::string>::from(v).transform([](const auto& s) { return std::filesystem::path{s}; });
}

///
///
template<value_castable T>
auto value_cast<std::optional<T>>::to(sol::state_view lua, const std::optional<T>& v) -> sol::object
{
  return v ? value_cast<T>::to(lua, *v) : sol::make_object(lua, sol::lua_nil);
}

///
///
template<value_castable T>
auto value_cast<std::optional<T>>::from(const sol::object& v) -> std::optional<std::optional<T>>
{
  if(v.get_type() == sol::type::lua_nil)
  {
    return std::optional<T>{};
  }
  return value_cast<T>::from(v).transform([](auto value) { return std::optional<T>{std::move(value)}; });
}

///
///
template<value_castable T>
auto value_cast<std::vector<T>>::to(sol::state_view lua, const std::vector<T>& v) -> sol::object
{
  auto table = lua.create_table(numeric_cast<int>(v.size()), 0);
  std::ranges::for_each(v, [&](const T& item) { table.add(value_cast<T>::to(lua, item)); });
  return table;
}

///
///
template<value_castable T>
auto value_cast<std::vector<T>>::from(const sol::object& v) -> std::optional<std::vector<T>>
{
  if(v.get_type() != sol::type::table)
  {
    return std::nullopt;
  }
  const auto table = v.as<sol::table>();
  // Else e.g. a table by key would be read as an empty list
  if(detail::entries(table).size() != table.size())
  {
    return std::nullopt;
  }
  return detail::all_or_none(
    util::ranges::index_view_between(std::size_t{1}, table.size() + 1) |
    std::views::transform([&](const std::size_t i) { return value_cast<T>::from(table.get<sol::object>(i)); })
  );
}

///
///
template<value_castable K, value_castable T>
auto value_cast<std::map<K, T>>::to(sol::state_view lua, const std::map<K, T>& v) -> sol::object
{
  auto table = lua.create_table(0, numeric_cast<int>(v.size()));
  std::ranges::for_each(
    v, [&](const auto& entry) { table[value_cast<K>::to(lua, entry.first)] = value_cast<T>::to(lua, entry.second); }
  );
  return table;
}

///
///
template<value_castable K, value_castable T>
auto value_cast<std::map<K, T>>::from(const sol::object& v) -> std::optional<std::map<K, T>>
{
  if(v.get_type() != sol::type::table)
  {
    return std::nullopt;
  }
  const auto entries =
    detail::entries(v.as<sol::table>()) |
    std::views::transform([](const auto& entry)
                          { return std::pair{value_cast<K>::from(entry.first), value_cast<T>::from(entry.second)}; }) |
    std::ranges::to<std::vector>();
  if(!std::ranges::all_of(entries, [](const auto& entry) { return entry.first && entry.second; }))
  {
    return std::nullopt;
  }
  return entries | std::views::transform([](const auto& entry) { return std::pair{*entry.first, *entry.second}; }) |
         std::ranges::to<std::map<K, T>>();
}

} // namespace bibstd::lua
