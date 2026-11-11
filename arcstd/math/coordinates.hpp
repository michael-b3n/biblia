#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <type_traits>

namespace bibstd::math
{

///
/// Coordinates for definition of a point in n-dim space.
///
template<typename ValueType, std::size_t D>
class coordinates final
{
  // Variables
  std::array<ValueType, D> coordinates_;

public: // Typedefs
  using value_type = ValueType;

public: // Static functions
  ///
  /// Get the distance between two coordinates.
  /// \return distance between `first` and `second`
  ///
  [[nodiscard]] static constexpr auto distance(const coordinates& first, const coordinates& second) -> double;

public: // Constructors
  template<typename... T>
  constexpr coordinates(T... coords)
    requires(sizeof...(T) == D);
  constexpr coordinates(std::array<value_type, D> coordinates);

public: // Accessors
  ///
  /// Get the coordinates value `x`.
  /// \return coordinate `x`
  ///
  [[nodiscard]] constexpr auto x() const -> value_type
    requires(D == 1 || D == 2 || D == 3);

  ///
  /// Get the coordinates value `y`.
  /// \return coordinate `y`
  ///
  [[nodiscard]] constexpr auto y() const -> value_type
    requires(D == 2 || D == 3);

  ///
  /// Get the coordinates value `z`.
  /// \return coordinate `z`
  ///
  [[nodiscard]] constexpr auto z() const -> value_type
    requires(D == 3);

  ///
  /// Get the coordinates value of the defined dimension number.
  /// \return coordinates value of dimension `dim`
  ///
  [[nodiscard]] constexpr auto axis_value(std::size_t dim) const -> value_type;

public: // Operators
  constexpr auto operator==(const coordinates<value_type, D>&) const -> bool = default;
  constexpr auto operator+(const coordinates<value_type, D>& rhs) const -> coordinates<value_type, D>;
  constexpr auto operator-(const coordinates<value_type, D>& rhs) const -> coordinates<value_type, D>;
};

///
///
template<typename... T>
coordinates(T...) -> coordinates<std::common_type_t<T...>, sizeof...(T)>;

///
/// 2D coordinates type definition.
///
template<typename T>
using coordinates_2d = coordinates<T, 2>;

///
/// 3D coordinates type definition.
///
template<typename T>
using coordinates_3d = coordinates<T, 3>;

///
///
template<typename ValueType, std::size_t D>
constexpr auto coordinates<ValueType, D>::distance(const coordinates& first, const coordinates& second) -> double
{
  const auto create_distances = [&]<std::size_t... I>([[maybe_unused]] std::index_sequence<I...>)
  { return std::array{static_cast<double>(first.axis_value(I)) - static_cast<double>(second.axis_value(I))...}; };
  const auto distances = create_distances(std::make_index_sequence<D>());
  const auto distance_squared =
    std::ranges::fold_left(distances, 0.0, [](const double first, const double second) { return first + std::pow(second, 2); });
  return std::sqrt(distance_squared);
}

///
///
template<typename ValueType, std::size_t D>
template<typename... T>
constexpr coordinates<ValueType, D>::coordinates(T... coords)
  requires(sizeof...(T) == D)
  : coordinates_{std::array{static_cast<value_type>(coords)...}}
{
}

///
///
template<typename ValueType, std::size_t D>
constexpr coordinates<ValueType, D>::coordinates(std::array<value_type, D> coordinates)
  : coordinates_{std::move(coordinates)}
{
}

///
///
template<typename ValueType, std::size_t D>
constexpr auto coordinates<ValueType, D>::x() const -> value_type
  requires(D == 1 || D == 2 || D == 3)
{
  return axis_value(0);
}

///
///
template<typename ValueType, std::size_t D>
constexpr auto coordinates<ValueType, D>::y() const -> value_type
  requires(D == 2 || D == 3)
{
  return axis_value(1);
}

///
///
template<typename ValueType, std::size_t D>
constexpr auto coordinates<ValueType, D>::z() const -> value_type
  requires(D == 3)
{
  return axis_value(2);
}

///
///
template<typename ValueType, std::size_t D>
constexpr auto coordinates<ValueType, D>::axis_value(const std::size_t dim) const -> value_type
{
  return coordinates_.at(dim);
}

///
///
template<typename ValueType, std::size_t D>
constexpr auto coordinates<ValueType, D>::operator+(const coordinates<value_type, D>& rhs) const -> coordinates<value_type, D>
{
  std::array<value_type, D> coords{};
  std::ranges::transform(coordinates_, rhs.coordinates_, coords.begin(), std::plus{});
  return coordinates(std::move(coords));
}

///
///
template<typename ValueType, std::size_t D>
constexpr auto coordinates<ValueType, D>::operator-(const coordinates<value_type, D>& rhs) const -> coordinates<value_type, D>
{
  std::array<value_type, D> coords{};
  std::ranges::transform(coordinates_, rhs.coordinates_, coords.begin(), std::minus{});
  return coordinates(std::move(coords));
}

} // namespace bibstd::math

///
///
template<typename ValueType>
struct std::formatter<bibstd::math::coordinates<ValueType, 2>> : std::formatter<std::string>
{
  auto format(const bibstd::math::coordinates<ValueType, 2>& e, std::format_context& ctx) const
  {
    return std::formatter<std::string>::format(std::format("({},{})", e.x(), e.y()), ctx);
  }
};

///
///
template<typename ValueType>
struct std::formatter<bibstd::math::coordinates<ValueType, 3>> : std::formatter<std::string>
{
  auto format(const bibstd::math::coordinates<ValueType, 3>& e, std::format_context& ctx) const
  {
    return std::formatter<std::string>::format(std::format("({},{},{})", e.x(), e.y(), e.z()), ctx);
  }
};
