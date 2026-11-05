#pragma once

#include <cmath>
#include <concepts>

namespace bibstd::math
{
namespace detail
{

///
/// Check if floating point value is not NaN and not Inf.
/// \tparam T Floating point type
/// \return true if valid, false otherwise
///
template<std::floating_point T>
[[nodiscard]] constexpr auto is_valid(const T value) -> bool
{
  const auto type_id = std::fpclassify(value);
  return type_id != FP_INFINITE && type_id != FP_NAN;
}

} // namespace detail

///
/// Check if floating point values are not NaN and not Inf.
/// \tparam T... Variadic floating point types
/// \return true if all are valid, false otherwise
///
template<std::floating_point... T>
[[nodiscard]] constexpr auto is_valid(const T... value) -> bool
{
  return (detail::is_valid(value) && ...);
}

} // namespace bibstd::math
