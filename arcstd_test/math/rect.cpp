#include <arcstd/math/is_equal.hpp>
#include <arcstd/math/rect.hpp>

#include <catch2/catch_test_macros.hpp>

namespace arcstd::math
{

TEST_CASE("rect", "[math]")
{
  GIVEN("empty double rect")
  {
    constexpr auto first = rect<double>(coordinates{1.0, 1.0}, 0.0, 0.0);
    constexpr auto second = rect<double>(coordinates{2.0, 2.0}, 0.0, 0.0);
    // size
    static_assert(math::is_equal(size(first.horizontal_range()), 0.0));
    static_assert(math::is_equal(size(first.vertical_range()), 0.0));
    // contains
    static_assert(!contains(first, second));
    static_assert(!contains(second, first));
    static_assert(contains(first, coordinates{1.0, 1.0}));
    static_assert(contains(second, coordinates{2.0, 2.0}));
    // overlap
    static_assert(!overlap(first, second));
    static_assert(math::surrounding_rect(first, second) == rect<double>(coordinates{1, 1}, 1u, 1u));
  }

  GIVEN("overlapping double rect")
  {
    constexpr auto first = rect<double>(coordinates{1.0, 1.0}, 2.0, 2.0);
    constexpr auto second = rect<double>(coordinates{2.0, 2.0}, 3.0, 3.0);
    // size
    static_assert(math::is_equal(size(first.horizontal_range()), 2.0));
    static_assert(math::is_equal(size(first.vertical_range()), 2.0));
    // contains
    static_assert(!contains(first, second));
    static_assert(!contains(second, first));
    static_assert(contains(first, coordinates{2.5, 2.7}));
    static_assert(!contains(first, coordinates{2.5, 4.7}));
    // overlap
    static_assert(overlap(first, second) == rect<double>(coordinates{2.0, 2.0}, 1.0, 1.0));
    static_assert(math::surrounding_rect(first, second) == rect<double>(coordinates{1, 1}, 4u, 4u));
  }

  GIVEN("containing double rect")
  {
    constexpr auto first = rect<double>(coordinates{1.0, 1.0}, 4.0, 4.0);
    constexpr auto second = rect<double>(coordinates{2.0, 2.0}, 3.0, 3.0);
    // contains
    static_assert(contains(first, second));
    static_assert(!contains(second, first));
    // overlap
    static_assert(overlap(first, second) == second);
    static_assert(math::surrounding_rect(first, second) == rect<double>(coordinates{1, 1}, 4u, 4u));
  }

  GIVEN("empty integer rect")
  {
    constexpr auto first = rect<int>(coordinates{1, 1}, 0u, 0u);
    constexpr auto second = rect<int>(coordinates{2, 2}, 0u, 0u);
    // size
    static_assert(math::is_equal(size(first.horizontal_range()), 0));
    static_assert(math::is_equal(size(first.vertical_range()), 0));
    // contains
    static_assert(!contains(first, second));
    static_assert(!contains(second, first));
    static_assert(!contains(first, coordinates{1, 1}));
    static_assert(!contains(second, coordinates{2, 2}));
    // overlap
    static_assert(!overlap(first, second));
    static_assert(math::surrounding_rect(first, second) == rect<int>(coordinates{0, 0}, 0u, 0u));
  }

  GIVEN("overlapping integer rect")
  {
    constexpr auto first = rect<int>(coordinates{1, 1}, 2u, 2u);
    constexpr auto second = rect<int>(coordinates{2, 2}, 3u, 3u);
    // size
    static_assert(math::is_equal(size(first.horizontal_range()), 2));
    static_assert(math::is_equal(size(first.vertical_range()), 2));
    // contains
    static_assert(!contains(first, second));
    static_assert(!contains(second, first));
    static_assert(contains(first, coordinates{2, 2}));
    static_assert(!contains(first, coordinates{2, 4}));
    // overlap
    static_assert(overlap(first, second) == rect<int>(coordinates{2, 2}, 1u, 1u));
    static_assert(math::surrounding_rect(first, second) == rect<int>(coordinates{1, 1}, 4u, 4u));
  }

  GIVEN("containing integer rect")
  {
    constexpr auto first = rect<int>(coordinates{1, 1}, 4u, 4u);
    constexpr auto second = rect<int>(coordinates{2, 2}, 3u, 3u);
    // contains
    static_assert(contains(first, second));
    static_assert(!contains(second, first));
    // overlap
    static_assert(overlap(first, second) == second);
    static_assert(math::surrounding_rect(first, second) == rect<int>(coordinates{1, 1}, 4u, 4u));
  }

  GIVEN("constructor variations")
  {
    [[maybe_unused]] constexpr auto e1 = rect<std::int64_t>(coordinates{std::int16_t{1}, 1}, std::uint32_t{4}, std::uint8_t{2});
    [[maybe_unused]] constexpr auto e2 = rect<std::uint64_t>(coordinates{std::uint8_t{2}, 2u}, std::uint16_t{4}, 3u);
    [[maybe_unused]] constexpr auto e3 = rect<double>(coordinates{std::uint8_t{2}, 2u}, std::uint16_t{4}, 3u);
    [[maybe_unused]] constexpr auto e4 = rect<double>(coordinates{1.0f, 2.0}, 4.0f, 3.0);
  }
}

} // namespace arcstd::math
