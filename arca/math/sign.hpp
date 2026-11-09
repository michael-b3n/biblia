#pragma once

#include <type_traits>

namespace bibstd::math
{

///
/// Signed type concept.
///
template<typename T>
concept signed_type = std::is_signed_v<T>;

///
/// Unsigned type concept.
///
template<typename T>
concept unsigned_type = std::is_unsigned_v<T>;

///
/// All available sign types in a struct.
///
struct sign_value final
{
  static constexpr int positive = 1;
  static constexpr int zero = 0;
  static constexpr int negative = -1;
};

///
/// Get the sign of an arithmetic value of type `T`.
/// \return `-1` if `value < 0`, `0` if `value == 0`, `1` if `value > 0`
///
template<typename T>
[[nodiscard]] constexpr auto sign(T value) -> int
  requires(std::is_unsigned_v<T>)
{
  return static_cast<int>(T{0} < value);
}

///
/// Get the sign of an arithmetic value of type `T`.
/// \return `-1` if `value < 0`, `0` if `value == 0`, `1` if `value > 0`
///
template<typename T>
[[nodiscard]] constexpr auto sign(T value) -> int
  requires(std::is_signed_v<T>)
{
  return static_cast<int>(T{0} < value) - static_cast<int>(value < T{0});
}

} // namespace bibstd::math
