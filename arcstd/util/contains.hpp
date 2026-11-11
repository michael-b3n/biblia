#pragma once

#include <algorithm>
#include <concepts>
#include <type_traits>

namespace bibstd::util
{

///
/// Check if an element is contained in a container.
/// \return true if `element` is found in container, false if not
///
[[nodiscard]] constexpr auto contains(const std::ranges::range auto& container, const auto& element) -> bool
  requires(std::equality_comparable_with<std::ranges::range_value_t<decltype(container)>, decltype(element)>)
{
  return std::ranges::find(container, element) != std::ranges::cend(container);
}

///
/// Check if an element is inside a container or not.
/// \return bool true if element is found in container, false otherwise
///
[[nodiscard]] constexpr auto contains(const std::ranges::range auto& container, const auto& pred) -> bool
  requires(
    !std::ranges::view<std::remove_cvref_t<decltype(container)>> &&
    std::predicate<decltype(pred), std::ranges::range_value_t<decltype(container)>>
  )
{
  return std::ranges::find_if(container, pred) != std::ranges::cend(container);
}

///
/// Check if an element is inside a container or not.
/// \return bool true if element is found in container, false otherwise
///
[[nodiscard]] constexpr auto contains(std::ranges::view auto view, const auto& pred) -> bool
  requires(std::predicate<decltype(pred), std::ranges::range_value_t<decltype(view)>>)
{
  return std::ranges::find_if(view, pred) != std::ranges::cend(view);
}

} // namespace bibstd::util
