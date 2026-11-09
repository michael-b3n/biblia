#pragma once

#include <concepts>
#include <ranges>
#include <type_traits>

namespace bibstd::util::ranges
{

///
/// Return an iota view over the whole range of the given range object.
/// \return iota view from 0 to size of range
///
template<std::ranges::sized_range R>
[[nodiscard]] constexpr auto index_view(const R& range) -> auto
{
  return std::views::iota(std::ranges::range_size_t<R>{0}, std::ranges::size(range));
}

///
/// Return an iota view from a start index to the end of the given range object.
/// If the begin index is larger then the range size, the view is empty.
/// \return iota view from given start index to size of range
///
template<std::ranges::sized_range R>
[[nodiscard]] constexpr auto index_view_from(const R& range, const std::ranges::range_size_t<R> begin) -> auto
{
  const auto size = std::ranges::size(range);
  return std::views::iota(std::min(begin, size), size);
}

///
/// Return an iota view from zero to the specified end index. Performs a max size check with the given range.
/// If the end index is larger then the range size, the view will go over the full range.
/// \return iota view from zero to the given end index or size of range if end is larger
///
template<std::ranges::sized_range R>
[[nodiscard]] constexpr auto index_view_to(const R& range, const std::ranges::range_size_t<R> end) -> auto
{
  const auto size = std::ranges::size(range);
  return std::views::iota(std::ranges::range_size_t<R>{0}, std::min(end, size));
}

///
/// Return an iota view from zero to the specified end index.
/// \return iota view from zero to the given end index
///
[[nodiscard]] constexpr auto index_view_to(const std::integral auto end) -> auto
{
  return std::views::iota(decltype(end){0}, std::max(decltype(end){0}, end));
}

///
/// Return an iota view from first to the specified end index.
/// \return iota view from first to the given end index
///
[[nodiscard]] constexpr auto index_view_between(const std::integral auto first, const std::integral auto second) -> auto
{
  using common_type = std::common_type_t<std::decay_t<decltype(first)>, std::decay_t<decltype(second)>>;
  return std::views::iota(
    std::min(static_cast<common_type>(first), static_cast<common_type>(second)),
    std::max(static_cast<common_type>(first), static_cast<common_type>(second))
  );
}

} // namespace bibstd::util::ranges
