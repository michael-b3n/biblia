#include <arcstd/util/numeric_cast.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

namespace arcstd::util
{

TEST_CASE("numeric_cast_rt at compile time", "[util]")
{
  static_assert(numeric_cast_rt<int>(1.0) == 1);
  static_assert(numeric_cast_rt<int>(1.9) == 1);
  static_assert(numeric_cast_rt<double>(1) == 1.0);
  static_assert(numeric_cast_rt<std::uint8_t>(255) == 255);
  static_assert(numeric_cast_rt<std::int8_t>(-1) == -1);
  // In a constant expression the cast is a plain static_cast, so it truncates instead of throwing.
  static_assert(numeric_cast_rt<std::uint8_t>(256) == 0);

  // Lvalues, const lvalues and rvalues are all accepted.
  static constexpr auto value = 1.9;
  static_assert(numeric_cast_rt<int>(value) == 1);
  static_assert(std::is_same_v<decltype(numeric_cast_rt<int>(value)), int>);
}

TEST_CASE("numeric_cast_rt at runtime", "[util]")
{
  CHECK(numeric_cast_rt<int>(1.9) == 1);
  CHECK(numeric_cast_rt<std::uint8_t>(255) == 255);
  CHECK(numeric_cast_rt<std::int64_t>(std::numeric_limits<std::int32_t>::max()) == 2147483647);

  auto lvalue = 1.9;
  CHECK(numeric_cast_rt<int>(lvalue) == 1);
  const auto const_lvalue = 1.9;
  CHECK(numeric_cast_rt<int>(const_lvalue) == 1);

  // At runtime an out of range value is reported instead of silently truncated.
  CHECK_THROWS_AS(numeric_cast_rt<std::uint8_t>(256), boost::numeric::positive_overflow);
  CHECK_THROWS_AS(numeric_cast_rt<std::uint8_t>(-1), boost::numeric::negative_overflow);
  CHECK_THROWS_AS(numeric_cast_rt<std::int8_t>(std::numeric_limits<std::int32_t>::max()), boost::numeric::positive_overflow);
}

TEST_CASE("numeric_cast macro", "[util]")
{
  CHECK(numeric_cast<int>(1.9) == 1);
  CHECK(numeric_cast<std::uint8_t>(255) == 255);
  CHECK_THROWS_AS(numeric_cast<std::uint8_t>(256), boost::numeric::bad_numeric_cast);
}

} // namespace arcstd::util
