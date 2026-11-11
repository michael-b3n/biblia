#include <arcstd/util/enum.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>

namespace bibstd::util
{

enum class enum_test_type
{
  a,
  b,
  c
};

enum class enum_test_offset_type : std::int8_t
{
  a = -1,
  b = 0,
  c = 1
};

TEST_CASE("enum_type concept", "[util]")
{
  static_assert(enum_type<enum_test_type>);
  static_assert(!enum_type<int>);
  static_assert(!enum_type<std::string_view>);
  static_assert(enum_type<enum_test_offset_type>);
}

TEST_CASE("enum to_integral and to_enum", "[util]")
{
  using enum enum_test_type;

  static_assert(to_integral(a) == 0);
  static_assert(to_integral(b) == 1);
  static_assert(to_integral(c) == 2);
  static_assert(to_enum<enum_test_type>(0) == a);
  static_assert(to_enum<enum_test_type>(2) == c);
  // Round trip through the underlying type.
  static_assert(to_enum<enum_test_type>(to_integral(b)) == b);

  static_assert(to_integral(enum_test_offset_type::a) == -1);
  static_assert(std::is_same_v<decltype(to_integral(enum_test_offset_type::a)), std::int8_t>);
}

TEST_CASE("enum names and values", "[util]")
{
  using enum enum_test_type;

  static_assert(enum_name(a) == "a");
  static_assert(enum_name(c) == "c");
  static_assert(enum_count<enum_test_type>() == 3);
  static_assert(enum_names<enum_test_type>() == std::array<std::string_view, 3>{"a", "b", "c"});
  static_assert(enum_values<enum_test_type>() == std::array{a, b, c});

  static_assert(enum_count<enum_test_offset_type>() == 3);
  static_assert(enum_name(enum_test_offset_type::a) == "a");
}

TEST_CASE("enum to_enum from name", "[util]")
{
  using enum enum_test_type;

  static_assert(to_enum<enum_test_type>("a") == a);
  static_assert(to_enum<enum_test_type>("c") == c);
  static_assert(to_enum<enum_test_type>("d") == std::nullopt);
  static_assert(to_enum<enum_test_type>("") == std::nullopt);
  // The lookup is case sensitive.
  static_assert(to_enum<enum_test_type>("A") == std::nullopt);
}

TEST_CASE("enum next and prev", "[util]")
{
  using enum enum_test_type;

  CHECK(next(a) == b);
  CHECK(next(b) == c);
  CHECK(prev(c) == b);
  CHECK(prev(b) == a);

  CHECK(has_next(a));
  CHECK(has_next(b));
  CHECK(!has_next(c));
  CHECK(has_prev(c));
  CHECK(has_prev(b));
  CHECK(!has_prev(a));

  CHECK(next(enum_test_offset_type::a) == enum_test_offset_type::b);
  CHECK(prev(enum_test_offset_type::b) == enum_test_offset_type::a);
  CHECK(!has_prev(enum_test_offset_type::a));
  CHECK(!has_next(enum_test_offset_type::c));
}

TEST_CASE("enum prev throws on underflow", "[util]")
{
  // The underlying type of enum_test_offset_type cannot hold a value below its minimum.
  CHECK_THROWS_AS(prev(static_cast<enum_test_offset_type>(std::numeric_limits<std::int8_t>::lowest())), util::exception);
  CHECK_THROWS_AS(next(static_cast<enum_test_offset_type>(std::numeric_limits<std::int8_t>::max())), util::exception);
}

TEST_CASE("enum valid", "[util]")
{
  using enum enum_test_type;

  CHECK(valid(a));
  CHECK(valid(b));
  CHECK(valid(c));
  CHECK(!valid(static_cast<enum_test_type>(3)));
  CHECK(!valid(static_cast<enum_test_type>(42)));

  CHECK(valid(enum_test_offset_type::a));
  CHECK(!valid(static_cast<enum_test_offset_type>(2)));
}

} // namespace bibstd::util
