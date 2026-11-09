#include <arca/meta/lossless_conversion.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <type_traits>

namespace bibstd::meta
{

TEST_CASE("is_lossless_conversion identity", "[meta]")
{
  static_assert(is_lossless_conversion_v<int, int>);
  static_assert(is_lossless_conversion_v<double, double>);
  static_assert(is_lossless_conversion_v<bool, bool>);
  // Cv-qualifiers do not change the outcome for identical types.
  static_assert(is_lossless_conversion_v<const int, int>);
  static_assert(is_lossless_conversion_v<int, const int>);
  // Non arithmetic types are never lossless convertible.
  static_assert(!is_lossless_conversion_v<void*, void*>);
}

TEST_CASE("is_lossless_conversion widening", "[meta]")
{
  static_assert(is_lossless_conversion_v<std::int8_t, std::int16_t>);
  static_assert(is_lossless_conversion_v<std::int8_t, std::int64_t>);
  static_assert(is_lossless_conversion_v<std::uint8_t, std::uint64_t>);
  static_assert(is_lossless_conversion_v<std::uint16_t, std::int32_t>);
  static_assert(is_lossless_conversion_v<std::int32_t, std::int64_t>);
  static_assert(is_lossless_conversion_v<std::int32_t, double>);
  static_assert(is_lossless_conversion_v<std::uint32_t, double>);
  static_assert(is_lossless_conversion_v<float, double>);
  static_assert(is_lossless_conversion_v<bool, double>);
}

TEST_CASE("is_lossless_conversion narrowing", "[meta]")
{
  static_assert(!is_lossless_conversion_v<std::int16_t, std::int8_t>);
  static_assert(!is_lossless_conversion_v<std::int64_t, std::int32_t>);
  static_assert(!is_lossless_conversion_v<double, float>);
  static_assert(!is_lossless_conversion_v<double, std::int64_t>);
  // A signed type never fits losslessly into an unsigned one and vice versa beyond the registered widenings.
  static_assert(!is_lossless_conversion_v<std::int8_t, std::uint16_t>);
  static_assert(!is_lossless_conversion_v<std::uint32_t, std::int32_t>);
  // 32 bit integers do not survive a trip through float.
  static_assert(!is_lossless_conversion_v<std::int32_t, float>);
  static_assert(!is_lossless_conversion_v<std::uint32_t, float>);
  static_assert(!is_lossless_conversion_v<std::int64_t, double>);
}

TEST_CASE("lossless_convertible concept", "[meta]")
{
  static_assert(lossless_convertible<std::int8_t, std::int32_t>);
  static_assert(!lossless_convertible<std::int32_t, std::int8_t>);
}

TEST_CASE("lossless_common_type single type", "[meta]")
{
  static_assert(std::is_same_v<lossless_common_type_t<int>, int>);
  static_assert(std::is_same_v<lossless_common_type_t<double>, double>);
  static_assert(std::is_same_v<lossless_common_type_t<const int>, int>);
  static_assert(std::is_void_v<lossless_common_type_t<void>>);
}

TEST_CASE("lossless_common_type same types", "[meta]")
{
  static_assert(std::is_same_v<lossless_common_type_t<int, int>, int>);
  static_assert(std::is_same_v<lossless_common_type_t<double, double, double>, double>);
  static_assert(std::is_same_v<lossless_common_type_t<const int, int>, int>);
}

TEST_CASE("lossless_common_type integrals", "[meta]")
{
  // If one type converts losslessly into the other, that other type is the common one.
  static_assert(std::is_same_v<lossless_common_type_t<std::int8_t, std::int32_t>, std::int32_t>);
  static_assert(std::is_same_v<lossless_common_type_t<std::int32_t, std::int8_t>, std::int32_t>);
  static_assert(std::is_same_v<lossless_common_type_t<std::uint8_t, std::uint64_t>, std::uint64_t>);

  // Mixing signedness needs a signed type wide enough for both.
  static_assert(std::is_same_v<lossless_common_type_t<std::uint8_t, std::int8_t>, std::int16_t>);
  static_assert(std::is_same_v<lossless_common_type_t<std::uint16_t, std::int16_t>, std::int32_t>);
  static_assert(std::is_same_v<lossless_common_type_t<std::uint32_t, std::int32_t>, std::int64_t>);
  // No integral type holds both the full uint64 and int64 ranges.
  static_assert(std::is_void_v<lossless_common_type_t<std::uint64_t, std::int64_t>>);
}

TEST_CASE("lossless_common_type floating points", "[meta]")
{
  static_assert(std::is_same_v<lossless_common_type_t<float, double>, double>);
  static_assert(std::is_same_v<lossless_common_type_t<double, float>, double>);
}

TEST_CASE("lossless_common_type mixed integral and floating point", "[meta]")
{
  static_assert(std::is_same_v<lossless_common_type_t<std::int8_t, float>, float>);
  static_assert(std::is_same_v<lossless_common_type_t<float, std::int8_t>, float>);
  // int32 does not fit into float, but it does fit into double.
  static_assert(std::is_same_v<lossless_common_type_t<std::int32_t, float>, double>);
  static_assert(std::is_same_v<lossless_common_type_t<float, std::int32_t>, double>);
  static_assert(std::is_same_v<lossless_common_type_t<std::int32_t, double>, double>);
  // int64 fits into no floating point type.
  static_assert(std::is_void_v<lossless_common_type_t<std::int64_t, double>>);
}

TEST_CASE("lossless_common_type variadic", "[meta]")
{
  static_assert(std::is_same_v<lossless_common_type_t<std::int8_t, std::int16_t, std::int32_t>, std::int32_t>);
  static_assert(std::is_same_v<lossless_common_type_t<std::int8_t, std::int16_t, float>, float>);
  static_assert(std::is_same_v<lossless_common_type_t<std::uint8_t, std::int8_t, std::int32_t>, std::int32_t>);
}

TEST_CASE("has_lossless_common_type concept", "[meta]")
{
  static_assert(has_lossless_common_type<std::int8_t, std::int32_t>);
  static_assert(has_lossless_common_type<std::uint32_t, std::int32_t>);
  static_assert(!has_lossless_common_type<std::uint64_t, std::int64_t>);
  static_assert(!has_lossless_common_type<std::int64_t, double>);
}

} // namespace bibstd::meta
