#include <arca/math/is_equal.hpp>
#include <arca/math/value_range.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibstd::math
{

TEST_CASE("value_range", "[math]")
{
  GIVEN("empty double value_range")
  {
    constexpr auto first_range = value_range<double>(1.0, 1.0);
    constexpr auto second_range = value_range<double>(2.0, 2.0);
    // size
    static_assert(math::is_equal(size(first_range), 0.0));
    static_assert(math::is_equal(size(second_range), 0.0));
    // contains
    static_assert(!contains(first_range, second_range));
    static_assert(!contains(second_range, first_range));
    static_assert(contains(first_range, 1.0));
    static_assert(!contains(first_range, 2.0));
    // overlap
    static_assert(!overlaps(first_range, second_range));
    static_assert(!overlap(first_range, second_range));
    // adjacent
    static_assert(!adjacent(first_range, second_range));
  }

  GIVEN("overlapping double value_range")
  {
    constexpr auto first_range = value_range<double>(0.0, 1.0);
    constexpr auto second_range = value_range<double>(0.5, 2.0);
    // size
    static_assert(math::is_equal(size(first_range), 1.0));
    static_assert(math::is_equal(size(second_range), 1.5));
    // contains
    static_assert(!contains(first_range, second_range));
    static_assert(!contains(second_range, first_range));
    static_assert(contains(first_range, 0.0));
    static_assert(contains(first_range, 1.0));
    static_assert(!contains(first_range, 2.0));
    // overlap
    static_assert(overlaps(first_range, second_range));
    static_assert(overlap(first_range, second_range) == value_range<double>(0.5, 1.0));
    // adjacent
    static_assert(!adjacent(first_range, second_range));
  }

  GIVEN("containing double value_range")
  {
    constexpr auto first_range = value_range<double>(0.5, 1.5);
    constexpr auto second_range = value_range<double>(0.5, 2.0);
    // size
    static_assert(math::is_equal(size(first_range), 1.0));
    static_assert(math::is_equal(size(second_range), 1.5));
    // contains
    static_assert(!contains(first_range, second_range));
    static_assert(contains(second_range, first_range));
    // overlap
    static_assert(overlaps(first_range, second_range));
    static_assert(overlap(first_range, second_range) == first_range);
    // adjacent
    static_assert(!adjacent(first_range, second_range));
  }

  GIVEN("adjacent double value_range")
  {
    constexpr auto first_range = value_range<double>(0.0, 1.5);
    constexpr auto second_range = value_range<double>(1.5, 2.0);
    // size
    static_assert(math::is_equal(size(first_range), 1.5));
    static_assert(math::is_equal(size(second_range), 0.5));
    // contains
    static_assert(!contains(first_range, second_range));
    static_assert(!contains(second_range, first_range));
    // overlap
    static_assert(overlaps(first_range, second_range));
    static_assert(overlap(first_range, second_range) == value_range<double>(1.5, 1.5));
    // adjacent
    static_assert(adjacent(first_range, second_range));
  }

  GIVEN("empty integer value_range")
  {
    constexpr auto first_range = value_range<int>(1, 1);
    constexpr auto second_range = value_range<int>(2, 2);
    // size
    static_assert(math::is_equal(size(first_range), 0));
    static_assert(math::is_equal(size(second_range), 0));
    // contains
    static_assert(!contains(first_range, second_range));
    static_assert(!contains(second_range, first_range));
    static_assert(!contains(first_range, 1));
    // overlap
    static_assert(!overlaps(first_range, second_range));
    static_assert(!overlap(first_range, second_range));
    // adjacent
    static_assert(!adjacent(first_range, second_range));
  }

  GIVEN("overlapping integer value_range")
  {
    constexpr auto first_range = value_range<int>(0, 2);
    constexpr auto second_range = value_range<int>(1, 3);
    // size
    static_assert(math::is_equal(size(first_range), 2));
    static_assert(math::is_equal(size(second_range), 2));
    // contains
    static_assert(!contains(first_range, second_range));
    static_assert(!contains(second_range, first_range));
    static_assert(!contains(first_range, -1));
    static_assert(contains(first_range, 0));
    static_assert(contains(first_range, 1));
    static_assert(!contains(first_range, 2));
    // overlap
    static_assert(overlaps(first_range, second_range));
    static_assert(overlap(first_range, second_range) == value_range<int>(1, 2));
    // adjacent
    static_assert(!adjacent(first_range, second_range));
  }

  GIVEN("containing integer value_range")
  {
    constexpr auto first_range = value_range<int>(1, 2);
    constexpr auto second_range = value_range<int>(1, 3);
    // size
    static_assert(math::is_equal(size(first_range), 1));
    static_assert(math::is_equal(size(second_range), 2));
    // contains
    static_assert(!contains(first_range, second_range));
    static_assert(contains(second_range, first_range));
    // overlap
    static_assert(overlaps(first_range, second_range));
    static_assert(overlap(first_range, second_range) == first_range);
    // adjacent
    static_assert(!adjacent(first_range, second_range));
  }

  GIVEN("adjacent integer value_range")
  {
    constexpr auto first_range = value_range<int>(0, 2);
    constexpr auto second_range = value_range<int>(2, 4);
    // size
    static_assert(math::is_equal(size(first_range), 2));
    static_assert(math::is_equal(size(second_range), 2));
    // contains
    static_assert(!contains(first_range, second_range));
    static_assert(!contains(second_range, first_range));
    // overlap
    static_assert(!overlaps(first_range, second_range));
    static_assert(!overlap(first_range, second_range));
    // adjacent
    static_assert(adjacent(first_range, second_range));
  }
}

} // namespace bibstd::math
