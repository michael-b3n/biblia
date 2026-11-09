#pragma once

#include "arca/util/ranges.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace bibstd::math
{

///
/// Generate all combinations of elements from a nested vector and apply a function to each combination.
/// The generation stops as soon as \p func returns false.
///
/// \code
/// const auto nested = std::vector<std::vector<int>>{{1, 2}, {3, 4, 5}};
///
/// for_each_combination(nested, [](const std::vector<int>& combination)
/// {
///   std::println("{}", combination);
///   return true; // return false to stop early
/// });
///
/// // prints:
/// // [1, 3]
/// // [2, 3]
/// // [1, 4]
/// // [2, 4]
/// // [1, 5]
/// // [2, 5]
/// \endcode
///
template<typename T, typename F>
  requires(std::is_invocable_r_v<bool, F, const std::vector<T>&>)
auto for_each_combination(const std::vector<std::vector<T>>& nested, F&& func) -> void
{
  if(nested.empty())
  {
    return;
  }

  const auto multipliers = [&]
  {
    auto result = std::vector<std::uint64_t>(nested.size(), 0);
    std::ranges::for_each(
      util::ranges::index_view(nested),
      [&](const auto i) { result.at(i) = (i > 0 ? result.at(i - 1) : 1) * nested.at(i).size(); }
    );
    return result;
  }();

  const auto generate_indexer = [&](const auto indexer_i)
  {
    const auto multiplier = multipliers.at(indexer_i);
    const auto size = nested.at(indexer_i).size();
    return [m = multiplier / size, size = size](const std::uint64_t i) { return (i / m) % size; };
  };

  const auto indexers =
    util::ranges::index_view(nested) | std::views::transform(generate_indexer) | std::ranges::to<std::vector>();

  auto args = std::vector<T>(nested.size());

  auto continue_flag = true;
  const auto combination_count = multipliers.back();
  std::ranges::for_each(
    util::ranges::index_view_to(combination_count) |
      std::views::take_while([&]([[maybe_unused]] const auto) { return continue_flag; }),
    [&](const auto i)
    {
      std::ranges::for_each(
        util::ranges::index_view(nested), [&](const auto j) { args.at(j) = nested.at(j).at(indexers.at(j)(i)); }
      );
      continue_flag = func(args);
    }
  );
}

} // namespace bibstd::math
