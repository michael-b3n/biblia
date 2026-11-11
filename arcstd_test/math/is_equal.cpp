#include <arcstd/math/is_equal.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>

namespace arcstd::math
{

TEST_CASE("is_equal floating point", "[math]")
{
  static_assert(is_equal(1.0, 1.0));
  static_assert(!is_equal(1.0, 1.5));
  static_assert(is_equal(0.0, -0.0));
  static_assert(is_equal(1.0f, 1.0f));

  // The default epsilon is too small to swallow a difference of 0.1.
  static_assert(!is_equal(1.0, 1.1));
  static_assert(is_equal(1.0, 1.1, 0.2));
  static_assert(!is_equal(1.0, 1.1, 0.05));

  // Epsilon is an exclusive bound.
  static_assert(!is_equal(1.0, 2.0, 1.0));
  static_assert(is_equal(1.0, 2.0, 1.5));

  // Order of the arguments does not matter.
  static_assert(is_equal(1.1, 1.0, 0.2));
  static_assert(is_equal(-1.0, -1.1, 0.2));
  static_assert(!is_equal(-1.0, 1.0, 0.2));
}

TEST_CASE("is_equal same signedness integrals", "[math]")
{
  static_assert(is_equal(1, 1));
  static_assert(!is_equal(1, 2));
  static_assert(is_equal(std::int8_t{-1}, std::int64_t{-1}));
  static_assert(is_equal(std::uint8_t{255}, std::uint64_t{255}));
  static_assert(!is_equal(std::uint8_t{255}, std::uint64_t{256}));
  static_assert(is_equal(std::numeric_limits<std::int32_t>::lowest(), std::numeric_limits<std::int32_t>::lowest()));
}

TEST_CASE("is_equal mixed signedness integrals", "[math]")
{
  static_assert(is_equal(1, 1u));
  static_assert(is_equal(1u, 1));
  static_assert(!is_equal(1, 2u));

  // A negative value is never equal to an unsigned one, whatever the argument order is.
  static_assert(!is_equal(-1, std::uint32_t{0}));
  static_assert(!is_equal(std::uint32_t{0}, -1));
  static_assert(!is_equal(-1, std::numeric_limits<std::uint32_t>::max()));
  static_assert(!is_equal(std::numeric_limits<std::uint32_t>::max(), -1));
  static_assert(!is_equal(std::numeric_limits<std::int64_t>::lowest(), std::uint64_t{0}));

  static_assert(is_equal(std::numeric_limits<std::int32_t>::max(), std::uint32_t{2147483647}));
  static_assert(!is_equal(std::numeric_limits<std::int32_t>::max(), std::uint32_t{2147483648}));
}

TEST_CASE("is_equal integral with comparable type", "[math]")
{
  enum class scoped_enum : int
  {
    a = 1,
    b = 2
  };
  // Scoped enums are not equality comparable with int, so only the char overloads apply here.
  static_assert(!std::equality_comparable_with<scoped_enum, int>);

  static_assert(is_equal('a', 'a'));
  static_assert(!is_equal('a', 'b'));
  static_assert(is_equal(std::int32_t{97}, 'a'));
  static_assert(is_equal('a', std::int32_t{97}));
  static_assert(!is_equal(std::int32_t{98}, 'a'));
}

TEST_CASE("is_equal non integral comparable type", "[math]")
{
  static_assert(is_equal(1, 1.0));
  static_assert(!is_equal(1, 1.5));
  static_assert(is_equal(1.0, 1));
  static_assert(!is_equal(1.5, 1));
}

} // namespace arcstd::math
