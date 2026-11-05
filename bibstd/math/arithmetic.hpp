#pragma once

#include "bibstd/math/sign.hpp"
#include "bibstd/util/exception.hpp"

#include <cassert>
#include <cmath>
#include <concepts>
#include <expected>
#include <limits>
#include <type_traits>

namespace bibstd::math
{

///
/// Arithmetic type concept.
///
template<typename T>
concept arithmetic_type = std::is_arithmetic_v<T>;

///
/// Concept checking if the value types are both integrals or floating points.
///
template<typename... T>
concept similar_arithmetic_types =
  (... && arithmetic_type<T>) && ((... && std::integral<T>) || (... && std::floating_point<T>));

///
/// Integer and floating point arithmetics guard for save math operations guaranteeing over- and underflow protection.
///
struct arithmetic final
{
  // Typedefs
  ///
  /// Arithmetics error code.
  ///
  enum class error_code
  {
    overflow,
    underflow,
    undefined
  };

  template<arithmetic_type T>
  using expected_type = std::expected<T, error_code>;

  // Static functions
  ///
  /// Add two values of an integer type `T`.
  /// \return result of `first + second`
  ///
  template<arithmetic_type T>
  [[nodiscard]] static constexpr auto add(T first, T second) -> expected_type<T>;

  ///
  /// Add two values of type `expected_type<T>`.
  /// \return result of `first + second`
  ///
  template<arithmetic_type T>
  [[nodiscard]] static constexpr auto add(expected_type<T> first, expected_type<T> second) -> expected_type<T>;

  ///
  /// Subtract two values of an integer type `T`.
  /// \return result of `first - second`
  ///
  template<arithmetic_type T>
  [[nodiscard]] static constexpr auto subtract(T first, T second) -> expected_type<T>;

  ///
  /// Subtract two values of type `expected_type<T>`.
  /// \return result of `first - second`
  ///
  template<arithmetic_type T>
  [[nodiscard]] static constexpr auto subtract(expected_type<T> first, expected_type<T> second) -> expected_type<T>;

  ///
  /// Multiply two values of an integer type `T`.
  /// \return result of `first * second`
  ///
  template<arithmetic_type T>
  [[nodiscard]] static constexpr auto multiply(T first, T second) -> expected_type<T>;

  ///
  /// Multiply two values of type `expected_type<T>`.
  /// \return result of `first * second`
  ///
  template<arithmetic_type T>
  [[nodiscard]] static constexpr auto multiply(expected_type<T> first, expected_type<T> second) -> expected_type<T>;

  ///
  /// Divide two values of an integer type `T`.
  /// \return result of `first / second`
  ///
  template<arithmetic_type T>
  [[nodiscard]] static constexpr auto divide(T first, T second) -> expected_type<T>;

  ///
  /// Divide two values of type `expected_type<T>`.
  /// \return result of `first / second`
  ///
  template<arithmetic_type T>
  [[nodiscard]] static constexpr auto divide(expected_type<T> first, expected_type<T> second) -> expected_type<T>;

  ///
  /// Clamp to lowest or max value depending on under- or overflow.
  /// Return std::nullopt if `value` parameter is undefined.
  /// \return clamped value
  ///
  template<arithmetic_type T>
  [[nodiscard]] static constexpr auto clamp(expected_type<T> value) -> std::optional<T>;
};

namespace detail
{

template<typename T>
struct arithmetic_operation;

template<std::unsigned_integral T>
struct arithmetic_operation<T>
{
  // Constants
  static constexpr auto min = std::numeric_limits<T>::lowest();
  static constexpr auto max = std::numeric_limits<T>::max();

  // Static functions
  [[nodiscard]] static constexpr auto add(T first, T second) -> arithmetic::expected_type<T>;
  [[nodiscard]] static constexpr auto subtract(T first, T second) -> arithmetic::expected_type<T>;
  [[nodiscard]] static constexpr auto multiply(T first, T second) -> arithmetic::expected_type<T>;
  [[nodiscard]] static constexpr auto divide(T first, T second) -> arithmetic::expected_type<T>;
};

template<std::signed_integral T>
struct arithmetic_operation<T>
{
  // Constants
  static constexpr auto min = std::numeric_limits<T>::lowest();
  static constexpr auto max = std::numeric_limits<T>::max();

  // Static functions
  [[nodiscard]] static constexpr auto add(T first, T second) -> arithmetic::expected_type<T>;
  [[nodiscard]] static constexpr auto subtract(T first, T second) -> arithmetic::expected_type<T>;
  [[nodiscard]] static constexpr auto multiply(T first, T second) -> arithmetic::expected_type<T>;
  [[nodiscard]] static constexpr auto divide(T first, T second) -> arithmetic::expected_type<T>;
};

template<std::floating_point T>
struct arithmetic_operation<T>
{
  // Constants
  static constexpr auto min = std::numeric_limits<T>::lowest();
  static constexpr auto max = std::numeric_limits<T>::max();

  // Static functions
  [[nodiscard]] static constexpr auto add(T first, T second) -> arithmetic::expected_type<T>;
  [[nodiscard]] static constexpr auto subtract(T first, T second) -> arithmetic::expected_type<T>;
  [[nodiscard]] static constexpr auto multiply(T first, T second) -> arithmetic::expected_type<T>;
  [[nodiscard]] static constexpr auto divide(T first, T second) -> arithmetic::expected_type<T>;
};

} // namespace detail

///
///
template<arithmetic_type T>
constexpr auto arithmetic::add(const T first, const T second) -> expected_type<T>
{
  return detail::arithmetic_operation<T>::add(first, second);
}

///
///
template<arithmetic_type T>
constexpr auto arithmetic::add(const expected_type<T> first, const expected_type<T> second) -> expected_type<T>
{
  using op = detail::arithmetic_operation<T>;
  return first.and_then([&](const T first_v)
                        { return second.and_then([&](const T second_v) { return op::add(first_v, second_v); }); });
}

///
///
template<arithmetic_type T>
constexpr auto arithmetic::subtract(const T first, const T second) -> expected_type<T>
{
  return detail::arithmetic_operation<T>::subtract(first, second);
}

///
///
template<arithmetic_type T>
constexpr auto arithmetic::subtract(const expected_type<T> first, const expected_type<T> second) -> expected_type<T>
{
  using op = detail::arithmetic_operation<T>;
  return first.and_then([&](const T first_v)
                        { return second.and_then([&](const T second_v) { return op::subtract(first_v, second_v); }); });
}

///
///
template<arithmetic_type T>
constexpr auto arithmetic::multiply(const T first, const T second) -> expected_type<T>
{
  return detail::arithmetic_operation<T>::multiply(first, second);
}

///
///
template<arithmetic_type T>
constexpr auto arithmetic::multiply(const expected_type<T> first, const expected_type<T> second) -> expected_type<T>
{
  using op = detail::arithmetic_operation<T>;
  return first.and_then([&](const T first_v)
                        { return second.and_then([&](const T second_v) { return op::multiply(first_v, second_v); }); });
}

///
///
template<arithmetic_type T>
constexpr auto arithmetic::divide(const T first, const T second) -> expected_type<T>
{
  return detail::arithmetic_operation<T>::divide(first, second);
}

///
///
template<arithmetic_type T>
constexpr auto arithmetic::divide(const expected_type<T> first, const expected_type<T> second) -> expected_type<T>
{
  using op = detail::arithmetic_operation<T>;
  return first.and_then([&](const T first_v)
                        { return second.and_then([&](const T second_v) { return op::divide(first_v, second_v); }); });
}

///
///
template<arithmetic_type T>
constexpr auto arithmetic::clamp(expected_type<T> value) -> std::optional<T>
{
  if(value.has_value())
  {
    return value.value();
  }
  else
  {
    switch(value.error())
    {
    case error_code::overflow: return std::numeric_limits<T>::max();
    case error_code::underflow: return std::numeric_limits<T>::lowest();
    case error_code::undefined: return std::nullopt;
    default: throw util::exception("unknown error code"); return T{0};
    }
  }
}

namespace detail
{

///
/// Classify floating point value and return corresponding expected type.
/// \return expected type dependent on `value` classification
///
template<std::floating_point T>
[[nodiscard]] constexpr auto floating_point_to_expected_type(T value) -> arithmetic::expected_type<T>
{
  switch(std::fpclassify(value))
  {
  case FP_INFINITE:
  {
    const auto inf_sign = sign(value);
    return inf_sign == sign_value::positive ? std::unexpected{arithmetic::error_code::overflow}
                                            : std::unexpected{arithmetic::error_code::underflow};
  }
  case FP_NAN: return std::unexpected{arithmetic::error_code::undefined};
  case FP_NORMAL: [[fallthrough]];
  case FP_SUBNORMAL: [[fallthrough]];
  case FP_ZERO: [[fallthrough]];
  default: return value;
  }
}

///
///
template<std::unsigned_integral T>
constexpr auto arithmetic_operation<T>::add(T first, T second) -> arithmetic::expected_type<T>
{
  if(first > max - second)
  {
    return std::unexpected{arithmetic::error_code::overflow};
  }
  else
  {
    return first + second;
  }
}

///
///
template<std::signed_integral T>
constexpr auto arithmetic_operation<T>::add(T first, T second) -> arithmetic::expected_type<T>
{
  const auto second_sign = sign(second);
  const auto overflow = second_sign == sign_value::positive && first > max - second;
  const auto underflow = second_sign == sign_value::negative && first < min - second;
  if(overflow)
  {
    return std::unexpected{arithmetic::error_code::overflow};
  }
  else if(underflow)
  {
    return std::unexpected{arithmetic::error_code::underflow};
  }
  else
  {
    return first + second;
  }
}

///
///
template<std::floating_point T>
constexpr auto arithmetic_operation<T>::add(T first, T second) -> arithmetic::expected_type<T>
{
  const auto result = first + second;
  return floating_point_to_expected_type(result);
}

///
///
template<std::unsigned_integral T>
constexpr auto arithmetic_operation<T>::subtract(T first, T second) -> arithmetic::expected_type<T>
{
  if(first < min + second)
  {
    return std::unexpected{arithmetic::error_code::underflow};
  }
  else
  {
    return first - second;
  }
}

///
///
template<std::signed_integral T>
constexpr auto arithmetic_operation<T>::subtract(T first, T second) -> arithmetic::expected_type<T>
{
  const auto second_sign = sign(second);
  const auto overflow = second_sign == sign_value::negative && first > max + second;
  const auto underflow = second_sign == sign_value::positive && first < min + second;
  if(overflow)
  {
    return std::unexpected{arithmetic::error_code::overflow};
  }
  else if(underflow)
  {
    return std::unexpected{arithmetic::error_code::underflow};
  }
  else
  {
    return first - second;
  }
}

///
///
template<std::floating_point T>
constexpr auto arithmetic_operation<T>::subtract(T first, T second) -> arithmetic::expected_type<T>
{
  const auto result = first - second;
  return floating_point_to_expected_type(result);
}

///
///
template<std::unsigned_integral T>
constexpr auto arithmetic_operation<T>::multiply(T first, T second) -> arithmetic::expected_type<T>
{
  if(second == T{0} || first == T{0})
  {
    return T{0};
  }
  else if(max % second == 0 && first > max / second)
  {
    return std::unexpected{arithmetic::error_code::overflow};
  }
  else if(first - T{1} > max / second)
  {
    return std::unexpected{arithmetic::error_code::overflow};
  }
  else
  {
    return first * second;
  }
}

///
///
template<std::signed_integral T>
constexpr auto arithmetic_operation<T>::multiply(T first, T second) -> arithmetic::expected_type<T>
{
  // `constexpr std::abs` wrapper, since std::abs(long long) of the standard library is not `constexpr`.
  constexpr auto abs = [](auto value) -> auto
  {
    if consteval
    {
      if constexpr(std::integral<decltype(value)>)
      {
        assert(value != std::numeric_limits<decltype(value)>::min());
        return value < 0 ? -value : value;
      }
    }
    return std::abs(value);
  };

  auto overflow = false;
  auto underflow = false;
  if(second == T{0} || first == T{0})
  {
    return T{0};
  }
  else if(first == min && second != T{1})
  {
    underflow = sign(second) == sign_value::positive;
    overflow = sign(second) == sign_value::negative;
  }
  else if(second == min && first != T{1})
  {
    underflow = sign(first) == sign_value::positive;
    overflow = sign(first) == sign_value::negative;
  }
  else if(abs(first) > T{1} && abs(second) > T{1})
  {
    const auto multiplied_sign = sign(first) * sign(second);
    overflow = multiplied_sign == sign_value::positive &&
               (max % abs(second) == 0 ? (abs(first) > max / abs(second)) : (abs(first) - T{1} > max / abs(second)));
    underflow = multiplied_sign == sign_value::negative &&
                (min % abs(second) == 0 ? (abs(first) > abs(min / abs(second))) : (abs(first) + T{1} > abs(min / abs(second))));
  }
  if(overflow)
  {
    return std::unexpected{arithmetic::error_code::overflow};
  }
  else if(underflow)
  {
    return std::unexpected{arithmetic::error_code::underflow};
  }
  else
  {
    return first * second;
  }
}

///
///
template<std::floating_point T>
constexpr auto arithmetic_operation<T>::multiply(T first, T second) -> arithmetic::expected_type<T>
{
  const auto result = first * second;
  return floating_point_to_expected_type(result);
}

///
///
template<std::unsigned_integral T>
constexpr auto arithmetic_operation<T>::divide(T first, T second) -> arithmetic::expected_type<T>
{
  if(first == T{0} && second == T{0})
  {
    return std::unexpected{arithmetic::error_code::undefined};
  }
  else if(first == T{0})
  {
    return T{0};
  }
  else if(second == T{0})
  {
    return std::unexpected{arithmetic::error_code::overflow};
  }
  else
  {
    return first / second;
  }
}

///
///
template<std::signed_integral T>
constexpr auto arithmetic_operation<T>::divide(T first, T second) -> arithmetic::expected_type<T>
{
  auto overflow = false;
  auto underflow = false;
  if(first == T{0} && second == T{0})
  {
    return std::unexpected{arithmetic::error_code::undefined};
  }
  else if(first == T{0})
  {
    return T{0};
  }
  else if(second == T{0})
  {
    sign(first) == sign_value::positive ? overflow = true : underflow = true;
  }
  else if(first == min && second == T{-1})
  {
    overflow = true;
  }
  if(overflow)
  {
    return std::unexpected{arithmetic::error_code::overflow};
  }
  else if(underflow)
  {
    return std::unexpected{arithmetic::error_code::underflow};
  }
  else
  {
    return first / second;
  }
}

///
///
template<std::floating_point T>
constexpr auto arithmetic_operation<T>::divide(T first, T second) -> arithmetic::expected_type<T>
{
  const auto result = first / second;
  return floating_point_to_expected_type(result);
}

} // namespace detail
} // namespace bibstd::math
