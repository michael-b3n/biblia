#pragma once

#include "arca/math/arithmetic.hpp"
#include "arca/math/coordinates.hpp"
#include "arca/math/value_range.hpp"
#include "arca/meta/lossless_conversion.hpp"
#include "arca/meta/type_traits.hpp"

#include <cassert>
#include <optional>
#include <type_traits>

namespace bibstd::math
{

///
/// Rectangle in a 2D plane on basis of an arithmetic value type.
/// Horizontal and vertical range definition for integer types: `[begin, end)`
/// Horizontal and vertical range definition for floating point: `[begin, end]`
///
template<arithmetic_type ValueType>
class rect final
{
  static_assert(!std::is_same_v<bool, std::remove_cvref_t<ValueType>>);

  // Variables
  value_range<ValueType> horizontal_range_;
  value_range<ValueType> vertical_range_;

public: // Typedefs
  using value_type = ValueType;
  using coordinates_type = coordinates<value_type, 2>;

public: // Constructors
  template<
    meta::lossless_convertible<ValueType> T1,
    meta::lossless_convertible<meta::conditional_unsigned_t<ValueType>> T2,
    meta::lossless_convertible<meta::conditional_unsigned_t<ValueType>> T3>
  constexpr rect(coordinates_2d<T1> origin, T2 width, T3 height)
    requires(math::similar_arithmetic_types<T1, T2, T3>);

  template<meta::lossless_convertible<ValueType> T1, meta::lossless_convertible<ValueType> T2>
  constexpr rect(coordinates_2d<T1> first, coordinates_2d<T2> second);

  template<meta::lossless_convertible<ValueType> T>
  constexpr rect(const rect<T>& other);

public: // Operators
  constexpr auto operator==(const rect&) const -> bool = default;

public: // Accessors
  ///
  /// Get the span from left to right of the rectangle.
  /// \return the horizontal span of the rect
  ///
  [[nodiscard]] constexpr auto horizontal_range() const -> value_range<value_type>;

  ///
  /// Get the span from lower to upper of the rectangle.
  /// \return the vertical span of the rect
  ///
  [[nodiscard]] constexpr auto vertical_range() const -> value_range<value_type>;

  ///
  /// Get the lower left coordinates of the rectangle.
  /// \return coordinates at lower left position
  ///
  [[nodiscard]] constexpr auto origin() const -> coordinates_type;

  ///
  /// Get the center coordinates of the rectangle.
  /// If there is no exact center, the closest lower left coordinates are returned.
  /// \return center coordinates
  ///
  [[nodiscard]] constexpr auto center() const -> coordinates_type;
};

///
///
template<arithmetic_type ValueType>
template<
  meta::lossless_convertible<ValueType> T1,
  meta::lossless_convertible<meta::conditional_unsigned_t<ValueType>> T2,
  meta::lossless_convertible<meta::conditional_unsigned_t<ValueType>> T3>
constexpr rect<ValueType>::rect(const coordinates_2d<T1> origin, const T2 width, const T3 height)
  requires(math::similar_arithmetic_types<T1, T2, T3>)
  : horizontal_range_{value_range<value_type>::create_from_begin_and_size(origin.x(), width)}
  , vertical_range_{value_range<value_type>::create_from_begin_and_size(origin.y(), height)}
{
}

///
///
template<arithmetic_type ValueType>
template<meta::lossless_convertible<ValueType> T1, meta::lossless_convertible<ValueType> T2>
constexpr rect<ValueType>::rect(const coordinates_2d<T1> first, const coordinates_2d<T2> second)
  : horizontal_range_{[&]
                      {
                        const auto min_x = std::min(static_cast<value_type>(first.x()), static_cast<value_type>(second.x()));
                        const auto max_x = std::max(static_cast<value_type>(first.x()), static_cast<value_type>(second.x()));
                        return value_range<value_type>{min_x, max_x};
                      }()}
  , vertical_range_{[&]
                    {
                      const auto min_y = std::min(static_cast<value_type>(first.y()), static_cast<value_type>(second.y()));
                      const auto max_y = std::max(static_cast<value_type>(first.y()), static_cast<value_type>(second.y()));
                      return value_range<value_type>{min_y, max_y};
                    }()}
{
}

///
///
template<arithmetic_type ValueType>
template<meta::lossless_convertible<ValueType> T>
constexpr rect<ValueType>::rect(const rect<T>& other)
  : horizontal_range_{other.horizontal_range()}
  , vertical_range_{other.vertical_range()}
{
}

///
///
template<arithmetic_type ValueType>
constexpr auto rect<ValueType>::horizontal_range() const -> value_range<value_type>
{
  return horizontal_range_;
}

///
///
template<arithmetic_type ValueType>
constexpr auto rect<ValueType>::vertical_range() const -> value_range<value_type>
{
  return vertical_range_;
}

///
///
template<arithmetic_type ValueType>
constexpr auto rect<ValueType>::origin() const -> coordinates_type
{
  return coordinates_type{horizontal_range_.begin, vertical_range_.begin};
}

///
///
template<arithmetic_type ValueType>
constexpr auto rect<ValueType>::center() const -> coordinates_type
{
  const auto horizontal_size = math::size(horizontal_range_);
  const auto horizontal_offset = static_cast<value_type>(horizontal_size / decltype(horizontal_size){2});
  const auto vertical_size = math::size(vertical_range_);
  const auto vertical_offset = static_cast<value_type>(vertical_size / decltype(vertical_size){2});
  return coordinates_type{horizontal_range_.begin + horizontal_offset, vertical_range_.begin + vertical_offset};
}

///
/// Check if rect is empty. A rectangle is empty if
/// either the horizontal or the vertical range is empty.
/// \return true if empty, false otherwise
///
template<arithmetic_type T>
[[nodiscard]] constexpr auto empty(const rect<T>& rectangle) -> bool
{
  return empty(rectangle.horizontal_range()) || empty(rectangle.vertical_range());
}

///
/// Get the overlap of two rectangles.
/// \return the overlap of two rectangles as a rectangle
///
template<arithmetic_type T1, arithmetic_type T2>
[[nodiscard]] constexpr auto overlap(const rect<T1>& first, const rect<T2>& second)
  -> std::optional<rect<meta::lossless_common_type_t<T1, T2>>>
  requires(similar_arithmetic_types<T1, T2> && meta::has_lossless_common_type<T1, T2>)
{
  const auto x_overlap = overlap(first.horizontal_range(), second.horizontal_range());
  const auto y_overlap = overlap(first.vertical_range(), second.vertical_range());
  if(x_overlap && y_overlap)
  {
    return rect<meta::lossless_common_type_t<T1, T2>>(
      coordinates{x_overlap->begin, y_overlap->begin}, size(*x_overlap), size(*y_overlap)
    );
  }
  return std::nullopt;
}

///
/// Check if subrectangle is contained by another rectangle.
/// \return true if subrectangle is contained in rectangle, false otherwise
///
template<arithmetic_type T1, arithmetic_type T2>
[[nodiscard]] constexpr auto contains(const rect<T1>& rectangle, const rect<T2>& subrectangle) -> bool
  requires(similar_arithmetic_types<T1, T2> && meta::has_lossless_common_type<T1, T2>)
{
  const auto x_contains = contains(rectangle.horizontal_range(), subrectangle.horizontal_range());
  const auto y_contains = contains(rectangle.vertical_range(), subrectangle.vertical_range());
  return x_contains && y_contains;
}

///
/// Check if coordinates are contained by another rectangle.
/// \return true if coordinates are contained in rectangle, false otherwise
///
template<arithmetic_type T1, arithmetic_type T2>
[[nodiscard]] constexpr auto contains(const rect<T1>& rectangle, const coordinates_2d<T2>& coordinates) -> bool
  requires(similar_arithmetic_types<T1, T2> && meta::has_lossless_common_type<T1, T2>)
{
  const auto x_contains = contains(rectangle.horizontal_range(), coordinates.x());
  const auto y_contains = contains(rectangle.vertical_range(), coordinates.y());
  return x_contains && y_contains;
}

///
/// Get a rectangle that surrounds all given rectangles.
/// If all rectangles have integral value types and are empty, an empty rectangle
/// with its origin at (0,0) is returned.
/// \return rectangle that surrounds all given rectangles
///
template<arithmetic_type... T>
[[nodiscard]] constexpr auto surrounding_rect(const rect<T>&... rects) -> rect<meta::lossless_common_type_t<T...>>
  requires(similar_arithmetic_types<T...> && meta::has_lossless_common_type<T...>)
{
  static_assert(sizeof...(rects) > 0);
  using type = meta::lossless_common_type_t<T...>;

  auto non_empty_range_found = false;
  auto min_x = std::numeric_limits<type>::max();
  auto min_y = std::numeric_limits<type>::max();
  auto max_x = std::numeric_limits<type>::lowest();
  auto max_y = std::numeric_limits<type>::lowest();
  const auto adjust_minmax = [&](const auto& range)
  {
    if constexpr(std::integral<typename std::remove_cvref_t<decltype(range)>::value_type>)
    {
      // skip empty integral ranges
      if(math::empty(range.horizontal_range()) || math::empty(range.vertical_range()))
      {
        return;
      }
    }
    non_empty_range_found = true;
    min_x = std::min(min_x, static_cast<type>(range.horizontal_range().begin));
    min_y = std::min(min_y, static_cast<type>(range.vertical_range().begin));
    max_x = std::max(max_x, static_cast<type>(range.horizontal_range().end));
    max_y = std::max(max_y, static_cast<type>(range.vertical_range().end));
  };
  (adjust_minmax(rects), ...);
  if(non_empty_range_found)
  {
    return rect<type>(coordinates{min_x, min_y}, coordinates{max_x, max_y});
  }
  else
  {
    return rect<type>(coordinates{type{0}, type{0}}, coordinates{type{0}, type{0}});
  }
}

} // namespace bibstd::math

///
///
template<bibstd::math::arithmetic_type ValueType>
struct std::formatter<bibstd::math::rect<ValueType>> : std::formatter<std::string>
{
  auto format(const bibstd::math::rect<ValueType>& e, std::format_context& ctx) const
  {
    return formatter<std::string>::format(std::format("[{} {}x{}]", e.origin(), e.horizontal_range(), e.vertical_range()), ctx);
  }
};
