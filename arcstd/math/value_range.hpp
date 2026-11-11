#pragma once

#include "arcstd/math/arithmetic.hpp"
#include "arcstd/math/is_equal.hpp"
#include "arcstd/math/sign.hpp"
#include "arcstd/meta/lossless_conversion.hpp"
#include "arcstd/meta/type_traits.hpp"
#include "arcstd/util/exception.hpp"

#include <algorithm>
#include <concepts>
#include <expected>
#include <limits>
#include <optional>
#include <type_traits>

namespace arcstd::math
{

///
/// Value range type.
/// Range definition for integer types: `[begin, end)`
/// Range definition for floating point: `[begin, end]`
///
template<arithmetic_type ValueType>
struct value_range final
{
  static_assert(!std::is_same_v<bool, std::remove_cvref_t<ValueType>>);

  // Typedefs
  using value_type = ValueType;

  // Variables
  value_type begin;
  value_type end;

  ///
  /// Range creator taking begin and size values.
  /// \throws util::exception if the provided values would create an overflow
  /// \tparam T1 Type that is lossless convertible to ValueType
  /// \tparam T2 Type that is lossless convertible to unsigned type of ValueType
  /// \return value range
  ///
  template<meta::lossless_convertible<ValueType> T1, meta::lossless_convertible<meta::conditional_unsigned_t<ValueType>> T2>
  [[nodiscard]] static constexpr auto create_from_begin_and_size(T1 begin, T2 size) -> value_range<ValueType>;

  // Constructor
  ///
  /// Value range constructor taking begin and end of a value range.
  /// Range definition for integer types: `[begin, end)`
  /// Range definition for floating point: `[begin, end]`
  ///
  template<meta::lossless_convertible<ValueType> T1, meta::lossless_convertible<ValueType> T2>
  constexpr value_range(T1 begin, T2 end);

  ///
  /// Copy constructor accepting value ranges that are lossless convertible to this range.
  ///
  template<meta::lossless_convertible<ValueType> T1>
  constexpr value_range(const value_range<T1>& other);

  // Operators
  constexpr auto operator==(const value_range& other) const -> bool;
};

///
///
template<arithmetic_type ValueType>
template<meta::lossless_convertible<ValueType> T1, meta::lossless_convertible<meta::conditional_unsigned_t<ValueType>> T2>
constexpr auto value_range<ValueType>::create_from_begin_and_size(T1 begin, T2 size) -> value_range<ValueType>
{
  const auto b = static_cast<value_type>(begin);
  auto e = arithmetic::expected_type<value_type>{std::unexpected{arithmetic::error_code::overflow}};
  if constexpr(std::integral<value_type>)
  {
    if constexpr(std::is_unsigned_v<value_type>)
    {
      e = arithmetic::add(b, static_cast<value_type>(size));
    }
    else
    {
      using unsigned_type = std::make_unsigned_t<value_type>;
      static constexpr auto max_value_utype = static_cast<unsigned_type>(std::numeric_limits<value_type>::max());
      const auto s = static_cast<unsigned_type>(size);
      if(s <= max_value_utype)
      {
        e = arithmetic::add(b, static_cast<value_type>(s));
      }
      else if(math::sign(b) == math::sign_value::negative)
      {
        if(b == std::numeric_limits<value_type>::lowest())
        {
          static constexpr auto abs_lowest = max_value_utype + 1;
          assert(s >= abs_lowest);
          // no check needed since this is always in range of value_type
          e = static_cast<value_type>(s - abs_lowest);
        }
        else
        {
          const auto abs_begin = static_cast<unsigned_type>(std::abs(b));
          assert(s > abs_begin);
          const auto s_positive = s - abs_begin;
          if(s_positive <= max_value_utype)
          {
            e = static_cast<value_type>(s_positive);
          }
          // else it overflows
        }
      }
      // else it overflows
    }
  }
  else // floating point type
  {
    e = arithmetic::add(b, static_cast<value_type>(size));
  }
  if(!e.has_value())
  {
    throw util::exception{"value range overflow"};
  }
  return value_range<value_type>{b, *e};
}

///
///
template<arithmetic_type ValueType>
template<meta::lossless_convertible<ValueType> T1, meta::lossless_convertible<ValueType> T2>
constexpr value_range<ValueType>::value_range(T1 begin_, T2 end_)
  : begin{std::min(static_cast<value_type>(begin_), static_cast<value_type>(end_))}
  , end{std::max(static_cast<value_type>(begin_), static_cast<value_type>(end_))}
{
  if(!arithmetic::subtract(end, begin).has_value() || !arithmetic::add(begin, static_cast<value_type>(1)).has_value())
  {
    throw util::exception("invalid value_range arguments");
  }
}

///
///
template<arithmetic_type ValueType>
template<meta::lossless_convertible<ValueType> T1>
constexpr value_range<ValueType>::value_range(const value_range<T1>& other)
  : begin{static_cast<value_type>(other.begin)}
  , end{static_cast<value_type>(other.end)}
{
}

///
///
template<arithmetic_type ValueType>
constexpr auto value_range<ValueType>::operator==(const value_range& other) const -> bool
{
  return is_equal(begin, other.begin) && is_equal(end, other.end);
}

///
/// Check if the `value_range` is empty (`begin == end`).
/// \return true if `value_range` is empty, false otherwise
///
template<arithmetic_type T>
[[nodiscard]] constexpr auto empty(const value_range<T>& range) -> bool
{
  return range.begin == range.end;
}

///
/// Get the size (defined as `size = end - begin` where `end > begin`) of a `value_range`.
/// \return size of `value_range` (type std::size_t for integer types, type double for floating point types)
///
template<arithmetic_type T>
[[nodiscard]] constexpr auto size(const value_range<T>& range) -> auto
{
  if constexpr(std::integral<T>)
  {
    return static_cast<std::make_unsigned_t<T>>(range.end - range.begin);
  }
  else
  {
    return static_cast<double>(range.end - range.begin);
  }
}

///
/// Checks if `subrange` is fully contained in `range`.
/// \return true, if `subrange` is fully contained in `range`
///
template<arithmetic_type T1, arithmetic_type T2>
[[nodiscard]] constexpr auto contains(const value_range<T1>& range, const value_range<T2>& subrange) -> bool
  requires(similar_arithmetic_types<T1, T2> && meta::has_lossless_common_type<T1, T2>)
{
  using type = meta::lossless_common_type_t<T1, T2>;
  if constexpr(std::integral<T1> || std::integral<T2>)
  {
    if((std::integral<T1> && empty(range)) || (std::integral<T2> && empty(subrange)))
    {
      return false;
    }
  }
  return static_cast<type>(range.begin) <= static_cast<type>(subrange.begin) &&
         static_cast<type>(range.end) >= static_cast<type>(subrange.end);
}

///
/// Checks if `value` is fully contained in `range`.
/// \return true, if `value` is fully contained in `range`
///
template<arithmetic_type T1, arithmetic_type T2>
[[nodiscard]] constexpr auto contains(const value_range<T1>& range, T2 value) -> bool
  requires(similar_arithmetic_types<T1, T2> && meta::has_lossless_common_type<T1, T2>)
{
  using type = meta::lossless_common_type_t<T1, T2>;
  if constexpr(std::integral<type>)
  {
    return !empty(range) && static_cast<type>(range.begin) <= static_cast<type>(value) &&
           static_cast<type>(range.end) > static_cast<type>(value);
  }
  else
  {
    return static_cast<type>(range.begin) <= static_cast<type>(value) &&
           static_cast<type>(range.end) >= static_cast<type>(value);
  }
}

///
/// Checks if two ranges overlap.
/// Empty integer ranges will never overlap with another range.
/// \return true, if two ranges overlap, false otherwise
///
template<arithmetic_type T1, arithmetic_type T2>
[[nodiscard]] constexpr auto overlaps(const value_range<T1>& first, const value_range<T2>& second) -> bool
  requires(similar_arithmetic_types<T1, T2> && meta::has_lossless_common_type<T1, T2>)
{
  using type = meta::lossless_common_type_t<T1, T2>;
  if constexpr(std::integral<T1> || std::integral<T2>)
  {
    if(empty(first) || empty(second))
    {
      return false;
    }
    else
    {
      return static_cast<type>(first.end) > static_cast<type>(second.begin) &&
             static_cast<type>(second.end) > static_cast<type>(first.begin);
    }
  }
  else
  {
    return static_cast<type>(first.begin) <= static_cast<type>(second.end) &&
           static_cast<type>(second.begin) <= static_cast<type>(first.end);
  }
}

///
/// Get the range identifying an overlap between two ranges.
/// \return optional range describing overlap of `first` and `second`, std::nullopt if no overlap is present
///
template<arithmetic_type T1, arithmetic_type T2>
[[nodiscard]] constexpr auto overlap(const value_range<T1>& first, const value_range<T2>& second)
  -> std::optional<value_range<meta::lossless_common_type_t<T1, T2>>>
  requires(similar_arithmetic_types<T1, T2> && meta::has_lossless_common_type<T1, T2>)
{
  using type = meta::lossless_common_type_t<T1, T2>;
  if(overlaps(first, second))
  {
    const auto begin = std::max(static_cast<type>(second.begin), static_cast<type>(first.begin));
    const auto end = std::min(static_cast<type>(second.end), static_cast<type>(first.end));
    return value_range<type>{begin, end};
  }
  return std::nullopt;
}

///
/// Checks if two ranges are adjacent end another.
/// Empty integer ranges will never be adjacent to another range.
/// \return true if adjacent, false otherwise
///
template<arithmetic_type T1, arithmetic_type T2>
[[nodiscard]] constexpr auto adjacent(const value_range<T1>& first, const value_range<T2>& second) -> bool
  requires(similar_arithmetic_types<T1, T2> && meta::has_lossless_common_type<T1, T2>)
{
  using type = meta::lossless_common_type_t<T1, T2>;
  if constexpr(std::integral<T1> || std::integral<T2>)
  {
    if(empty(first) || empty(second))
    {
      return false;
    }
    else
    {
      return is_equal(static_cast<type>(first.begin), static_cast<type>(second.end)) ||
             is_equal(static_cast<type>(second.begin), static_cast<type>(first.end));
    }
  }
  else
  {
    return is_equal(static_cast<type>(first.begin), static_cast<type>(second.end)) ||
           is_equal(static_cast<type>(second.begin), static_cast<type>(first.end));
  }
}

///
/// Clamp value to be contained within range.
/// \return clamped value
///
template<arithmetic_type T>
[[nodiscard]] constexpr auto clamp(const value_range<T>& range, T value) -> T
{
  if constexpr(std::integral<T>)
  {
    if(empty(range))
    {
      throw util::exception{"cannot clamp to empty value_range"};
    }
    return std::clamp(value, range.begin, range.end - 1);
  }
  else
  {
    return std::clamp(value, range.begin, range.end);
  }
}

} // namespace arcstd::math

///
///
template<arcstd::math::arithmetic_type ValueType>
struct std::formatter<arcstd::math::value_range<ValueType>> : std::formatter<std::string>
{
  auto format(const arcstd::math::value_range<ValueType>& e, std::format_context& ctx) const
  {
    if constexpr(std::integral<ValueType>)
    {
      return formatter<std::string>::format(std::format("[{}, {})", e.begin, e.end), ctx);
    }
    else
    {
      return formatter<std::string>::format(std::format("[{}, {}]", e.begin, e.end), ctx);
    }
  }
};
