#pragma once

#include "arcstd/math/arithmetic.hpp"

#include <magic_enum/magic_enum.hpp>

#include <string>
#include <type_traits>

namespace arcstd::util
{

///
/// Concept to check if E is enum.
///
template<typename E>
concept enum_type = std::is_enum_v<E>;

///
/// Converts enum to integral.
/// \tparam E enum type
/// \return integral value corresponding to enum value
///
template<enum_type E>
[[nodiscard]] constexpr auto to_integral(const E e) -> std::underlying_type_t<E>
{
  return static_cast<std::underlying_type_t<E>>(e);
}

///
/// Converts enum to string.
/// \tparam E enum type
/// \return string_view name corresponding to enum value
///
template<enum_type E>
[[nodiscard]] constexpr auto enum_name(const E e) -> std::string_view
{
  return magic_enum::enum_name(e);
}

///
/// Get the count of enum values for a given enum type.
/// \tparam E enum type
/// \return count of enum values
///
template<enum_type E>
consteval auto enum_count() -> std::size_t
{
  return magic_enum::enum_count<E>();
}

///
/// Get all the enum names for a given enum type.
/// \tparam E enum type
/// \return array of enum value names
///
template<enum_type E>
consteval auto enum_names() -> std::array<std::string_view, enum_count<E>()>
{
  return magic_enum::enum_names<E>();
}

///
/// Get all the enum values for a given enum type.
/// \tparam E enum type
/// \return array of enum values
///
template<enum_type E>
consteval auto enum_values() -> std::array<E, enum_count<E>()>
{
  return magic_enum::enum_values<E>();
}

///
/// Converts integral to enum.
/// \tparam E enum type
/// \return enum type value
///
template<enum_type E>
[[nodiscard]] constexpr auto to_enum(const std::underlying_type_t<E> integral) -> E
{
  return static_cast<E>(integral);
}

///
/// Converts string view enum name to enum.
/// \tparam E enum type
/// \return optional enum type value, std::nullopt of name does not correspond to any enum value in specified enum type
///
template<enum_type E>
[[nodiscard]] constexpr auto to_enum(const std::string_view name) -> std::optional<E>
{
  const auto value = magic_enum::enum_cast<E>(name);
  return value ? *value : std::optional<E>{std::nullopt};
}

///
/// Get next enum value.
/// \tparam E enum type
/// \return next enum value of param e
///
template<enum_type E>
[[nodiscard]] constexpr auto next(const E e) -> E
{
  const auto value = math::arithmetic::add(to_integral(e), std::underlying_type_t<E>(1));
  if(!value.has_value())
  {
    throw util::exception("invalid next enum value");
  }
  return to_enum<E>(*value);
}

///
/// Checks if next enum value exists.
/// \tparam E enum type
/// \return flag indicating if next enum value exists
///
template<enum_type E>
[[nodiscard]] constexpr auto has_next(const E e) -> bool
{
  const auto value = math::arithmetic::add(to_integral(e), std::underlying_type_t<E>(1));
  return value.has_value() && magic_enum::enum_cast<E>(*value).has_value();
}

///
/// Get previous enum value.
/// \tparam E enum type
/// \return previous enum value of param e
///
template<enum_type E>
[[nodiscard]] constexpr auto prev(const E e) -> E
{
  const auto value = math::arithmetic::subtract(to_integral(e), std::underlying_type_t<E>(1));
  if(!value.has_value())
  {
    throw util::exception("invalid previous enum value");
  }
  return to_enum<E>(*value);
}

///
/// Checks if previous enum value exists.
/// \tparam E enum type
/// \return flag indicating if previous enum value exists
///
template<enum_type E>
[[nodiscard]] constexpr auto has_prev(const E e) -> bool
{
  const auto value = math::arithmetic::subtract(to_integral(e), std::underlying_type_t<E>(1));
  return value.has_value() && magic_enum::enum_cast<E>(*value).has_value();
}

///
/// Checks if enum value of fenced enum type is valid.
/// \tparam E fenced enum type
/// \return true if valid, false otherwise
///
template<enum_type E>
[[nodiscard]] constexpr auto valid(E e) -> bool
{
  return magic_enum::enum_cast<E>(to_integral(e)).has_value();
}

} // namespace arcstd::util
