#pragma once

#include "bibstd/math/arithmetic.hpp"
#include "bibstd/math/rect.hpp"
#include "bibstd/meta/lossless_conversion.hpp"
#include "bibstd/util/ranges.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <vector>

namespace bibstd::data
{
namespace detail
{

///
/// Helper base class for plane data types.
///
template<typename ContainerType>
class plane_base
{
  // Variables
  std::uint32_t width_{0};
  std::uint32_t height_{0};

protected: // Variables
  ContainerType data_{};

public: // Typedefs
  using size_type = std::uint32_t;
  using data_type = ContainerType;
  using value_type = typename data_type::value_type;
  using const_iterator = typename data_type::const_iterator;
  using const_reverse_iterator = typename data_type::const_reverse_iterator;

public: // Structors
  constexpr plane_base(size_type width, size_type height, data_type data);
  constexpr plane_base() = default;
  virtual ~plane_base() noexcept = default;

public: // Accessors
  constexpr auto width() const -> size_type { return width_; };
  constexpr auto height() const -> size_type { return height_; };
  constexpr auto at(std::size_t index) const -> const value_type& { return data_.at(index); };
  constexpr auto at(std::size_t index) -> value_type& { return data_.at(index); };
  constexpr auto size() const -> std::size_t { return static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_); };
  constexpr auto empty() const -> bool { return size() == 0; };

public: // Iterators
  constexpr auto begin() const -> const_iterator { return data_.begin(); }
  constexpr auto cbegin() const -> const_iterator { return data_.cbegin(); }
  constexpr auto end() const -> const_iterator { return data_.end(); }
  constexpr auto cend() const -> const_iterator { return data_.cend(); }
  constexpr auto rbegin() const -> const_reverse_iterator { return data_.rbegin(); }
  constexpr auto crbegin() const -> const_reverse_iterator { return data_.crbegin(); }
  constexpr auto rend() const -> const_reverse_iterator { return data_.rend(); }
  constexpr auto crend() const -> const_reverse_iterator { return data_.crend(); }
};

///
///
template<typename ContainerType>
constexpr plane_base<ContainerType>::plane_base(const size_type width, const size_type height, data_type data)
  : width_{width}
  , height_{height}
  , data_{std::move(data)}
{
}

} // namespace detail

///
/// Represents a plane of objects of type T with compile time known width and height.
/// The data ordering is canonical: first width (fast axis), then height (slow axis).
///
template<typename T, std::uint32_t W, std::uint32_t H>
class plane_const final : public detail::plane_base<std::array<T, static_cast<std::size_t>(W) * static_cast<std::size_t>(H)>>
{
  // Typedefs
  using base_type = detail::plane_base<std::array<T, static_cast<std::size_t>(W) * static_cast<std::size_t>(H)>>;

public: // Structors
  constexpr plane_const(base_type::data_type data);
  constexpr plane_const()
    requires(std::is_default_constructible_v<T>)
  = default;
};

///
/// Represents a plane of objects of type T with a width, height and a vector of data.
/// The data ordering is canonical: first width (fast axis), then height (slow axis).
///
template<typename T>
class plane final : public detail::plane_base<std::vector<T>>
{
  // Typedefs
  using base_type = detail::plane_base<std::vector<T>>;

public: // Structors
  constexpr plane(base_type::size_type width, base_type::size_type height);
  constexpr plane() = default;
};

///
/// View on width * height objects of type T, the plane must outlive the view.
/// \see plane<T>
///
template<typename T>
class plane_view final : public detail::plane_base<std::span<T>>
{
  // Typedefs
  using base_type = detail::plane_base<std::span<T>>;

public: // Typedefs
  using area_type = math::rect<std::int64_t>;

public: // Structors
  template<typename C>
  constexpr plane_view(const detail::plane_base<C>& p);
  template<typename C>
  constexpr plane_view(detail::plane_base<C>& p);
  constexpr plane_view() = default;

public: // Accessors
  ///
  /// Creates a ranges view on data corresponding to the provided subarea.
  /// \return ranges view iterating over the elements within the area
  ///
  template<meta::lossless_convertible<std::int64_t> I>
  constexpr auto data_view(math::rect<I> area) const -> auto;

  ///
  /// Access the size of the view corresponding to the provided subarea.
  /// \return ranges view size
  ///
  template<meta::lossless_convertible<std::int64_t> I>
  constexpr auto data_view_size(math::rect<I> area) const -> std::uint64_t;
};

///
///
template<typename T, std::uint32_t W, std::uint32_t H>
constexpr plane_const<T, W, H>::plane_const(base_type::data_type data)
  : base_type{W, H, std::move(data)}
{
}

///
///
template<typename T>
constexpr plane<T>::plane(const base_type::size_type width, const base_type::size_type height)
  : base_type{width, height, typename base_type::data_type(static_cast<std::size_t>(width) * static_cast<std::size_t>(height))}
{
}

///
///
template<typename T>
template<typename C>
constexpr plane_view<T>::plane_view(const detail::plane_base<C>& p)
  : base_type{p.width(), p.height(), typename base_type::data_type(p.cbegin(), p.cend())}
{
}

///
///
template<typename T>
template<typename C>
constexpr plane_view<T>::plane_view(detail::plane_base<C>& p)
  : base_type{p.width(), p.height(), typename base_type::data_type(p.begin(), p.end())}
{
}

///
///
template<typename T>
template<meta::lossless_convertible<std::int64_t> I>
constexpr auto plane_view<T>::data_view(const math::rect<I> area) const -> auto
{
  using area_type = math::rect<std::int64_t>;

  const auto img = area_type(math::coordinates{0, 0}, base_type::width(), base_type::height());
  const auto [size, origin, hr] = [&]
  {
    if(const auto overlap = math::overlap(area, img))
    {
      const auto hr = static_cast<std::uint64_t>(math::size(overlap->horizontal_range()));
      const auto vr = static_cast<std::uint64_t>(math::size(overlap->vertical_range()));
      return std::make_tuple(math::arithmetic::multiply(hr, vr).value(), overlap->origin(), hr);
    }
    return std::make_tuple(std::uint64_t{0}, typename area_type::coordinates_type(0, 0), std::uint64_t{0});
  }();

  const auto to_coord = [origin, hr](const auto i)
  { return typename area_type::coordinates_type(origin.x() + (i % hr), origin.y() + (i / hr)); };
  const auto from_coord = [w = base_type::width()](const auto& coord) { return (coord.y() * w) + coord.x(); };

  return util::ranges::index_view_to(size) | std::views::transform([to_coord, from_coord, d = base_type::data_](const auto i)
                                                                   { return d.at(from_coord(to_coord(i))); });
}

///
///
template<typename T>
template<meta::lossless_convertible<std::int64_t> I>
constexpr auto plane_view<T>::data_view_size(const math::rect<I> area) const -> std::uint64_t
{
  using area_type = math::rect<std::int64_t>;

  const auto img = area_type(math::coordinates{0, 0}, base_type::width(), base_type::height());
  const auto overlap = math::overlap(area, img);
  if(overlap)
  {
    return math::arithmetic::multiply(
             static_cast<std::uint64_t>(math::size(overlap->horizontal_range())),
             static_cast<std::uint64_t>(math::size(overlap->vertical_range()))
    )
      .value();
  }
  else
  {
    return static_cast<std::uint64_t>(0);
  }
}

} // namespace bibstd::data
