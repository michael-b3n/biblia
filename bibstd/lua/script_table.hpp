#pragma once

#include "bibstd/lua/value_cast.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/string_literal.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace bibstd::lua
{

///
/// Field \p Key of a script_table, a value of type \p T.
///
template<util::string_literal Key, value_castable T>
struct field final
{
  // Typedefs
  using type = T;

  // Constants
  static constexpr auto key = Key.view();
};

///
/// Field of a script_table: its key and the type of its value, \see field.
///
template<typename F>
concept script_field = requires {
  { F::key } -> std::convertible_to<std::string_view>;
  typename F::type;
} && value_castable<typename F::type>;

///
/// Table of the values of \p Fields, a Lua table with a key per field for scripts.
///
template<script_field... Fields>
class script_table final
{
  // Typedefs
  using values_type = std::tuple<typename Fields::type...>;

  // Variables
  values_type values_;

public: // Structors
  script_table() = default;
  script_table(typename Fields::type... values)
    requires(sizeof...(Fields) > 0);

public: // Operators
  auto operator==(const script_table& other) const -> bool = default;

public: // Constants
  static constexpr auto keys = std::array<std::string_view, sizeof...(Fields)>{Fields::key...};

public: // Accessors
  ///
  /// \return the value of the field \p Key
  ///
  template<util::string_literal Key>
  [[nodiscard]] auto get() const -> const auto&;

  ///
  /// \return the value of the field \p Key
  ///
  template<util::string_literal Key>
  [[nodiscard]] auto get() -> auto&;

  ///
  /// \return const references of the values of all fields
  ///
  [[nodiscard]] auto values() const -> const values_type&;

  ///
  /// \return references to the values of all fields
  ///
  [[nodiscard]] auto values() -> values_type&;

private: // Implementation
  template<util::string_literal Key>
  [[nodiscard]] static consteval auto index() -> std::size_t;
};

///
/// A table with the keys of the fields. Reading, nil is an empty table and a key of no field makes it none.
///
template<script_field... Fields>
struct value_cast<script_table<Fields...>> final
{
  // Typedefs
  using table_type = script_table<Fields...>;

  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(sol::state_view lua, const table_type& v) -> sol::object;
  [[nodiscard]] static auto from(const sol::object& v) -> std::optional<table_type>;

private: // Implementation
  template<typename T>
  [[nodiscard]] static auto assign(T& target, const sol::object& v) -> bool;
};

///
///
template<script_field... Fields>
script_table<Fields...>::script_table(typename Fields::type... values)
  requires(sizeof...(Fields) > 0)
  : values_{std::move(values)...}
{
}

///
///
template<script_field... Fields>
template<util::string_literal Key>
auto script_table<Fields...>::get() const -> const auto&
{
  return std::get<index<Key>()>(values_);
}

///
///
template<script_field... Fields>
template<util::string_literal Key>
auto script_table<Fields...>::get() -> auto&
{
  return std::get<index<Key>()>(values_);
}

///
///
template<script_field... Fields>
auto script_table<Fields...>::values() const -> const values_type&
{
  return values_;
}

///
///
template<script_field... Fields>
auto script_table<Fields...>::values() -> values_type&
{
  return values_;
}

///
///
template<script_field... Fields>
template<util::string_literal Key>
consteval auto script_table<Fields...>::index() -> std::size_t
{
  const auto it = std::ranges::find(keys, Key.view());
  if(it == std::ranges::end(keys))
  {
    throw util::exception{"key not found in script_table"};
  }
  return static_cast<std::size_t>(std::ranges::distance(std::ranges::begin(keys), it));
}

///
///
template<script_field... Fields>
auto value_cast<script_table<Fields...>>::to(sol::state_view lua, const table_type& v) -> sol::object
{
  auto table = lua.create_table(0, static_cast<int>(sizeof...(Fields)));
  [&]<std::size_t... I>(std::index_sequence<I...>)
  {
    ((table[std::string{Fields::key}] = value_cast<typename Fields::type>::to(lua, std::get<I>(v.values()))), ...);
  }(std::index_sequence_for<Fields...>{});
  return table;
}

///
///
template<script_field... Fields>
auto value_cast<script_table<Fields...>>::from(const sol::object& v) -> std::optional<table_type>
{
  auto lua = sol::state_view{v.lua_state()};
  const auto nil = v.get_type() == sol::type::lua_nil;
  if(!nil && v.get_type() != sol::type::table)
  {
    return std::nullopt;
  }
  const auto table = nil ? lua.create_table() : v.as<sol::table>();
  const auto known = std::ranges::all_of(
    detail::entries(table),
    [](const auto& entry)
    {
      const auto name = value_cast<std::string>::from(entry.first);
      return name && std::ranges::contains(table_type::keys, std::string_view{*name});
    }
  );
  auto result = table_type{};
  const auto read = [&]<std::size_t... I>(std::index_sequence<I...>)
  { return (assign(std::get<I>(result.values()), table.template get<sol::object>(std::string{Fields::key})) && ...); };
  return known && read(std::index_sequence_for<Fields...>{}) ? std::optional{std::move(result)} : std::nullopt;
}

///
///
template<script_field... Fields>
template<typename T>
auto value_cast<script_table<Fields...>>::assign(T& target, const sol::object& v) -> bool
{
  auto value = value_cast<T>::from(v);
  if(value)
  {
    target = std::move(*value);
  }
  return value.has_value();
}

} // namespace bibstd::lua
