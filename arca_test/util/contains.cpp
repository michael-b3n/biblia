#include <arca/util/contains.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::util
{

TEST_CASE("contains element", "[util]")
{
  static constexpr auto values = std::array{1, 2, 3};
  static_assert(contains(values, 1));
  static_assert(contains(values, 3));
  static_assert(!contains(values, 4));
  static_assert(!contains(std::array<int, 0>{}, 1));

  // The element only needs to be equality comparable with the range value type.
  static constexpr auto strings = std::array{std::string_view{"a"}, std::string_view{"b"}};
  static_assert(contains(strings, std::string_view{"a"}));
  static_assert(!contains(strings, std::string_view{"c"}));

  CHECK(contains(std::vector{1, 2, 3}, 2));
  CHECK(!contains(std::vector{1, 2, 3}, 0));
  CHECK(contains(std::vector<std::string>{"a", "b"}, std::string{"b"}));
}

TEST_CASE("contains predicate", "[util]")
{
  static constexpr auto values = std::array{1, 2, 3};
  static_assert(contains(values, [](const auto e) { return e == 2; }));
  static_assert(!contains(values, [](const auto e) { return e > 3; }));
  static_assert(contains(values, [](const auto e) { return e % 2 == 0; }));
  static_assert(!contains(std::array<int, 0>{}, [](const auto) { return true; }));
}

TEST_CASE("contains predicate on view", "[util]")
{
  static constexpr auto values = std::array{1, 2, 3, 4};
  const auto even = values | std::views::filter([](const auto e) { return e % 2 == 0; });
  CHECK(contains(even, [](const auto e) { return e == 2; }));
  CHECK(!contains(even, [](const auto e) { return e == 3; }));

  const auto empty = values | std::views::filter([](const auto) { return false; });
  CHECK(!contains(empty, [](const auto) { return true; }));
}

} // namespace bibstd::util
