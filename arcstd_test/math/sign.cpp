#include <arcstd/math/sign.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>

namespace arcstd::math
{

TEST_CASE("sign of signed integrals", "[math]")
{
  static_assert(sign(std::int8_t{0}) == sign_value::zero);
  static_assert(sign(std::int8_t{1}) == sign_value::positive);
  static_assert(sign(std::int8_t{-1}) == sign_value::negative);
  static_assert(sign(std::numeric_limits<std::int16_t>::max()) == sign_value::positive);
  static_assert(sign(std::numeric_limits<std::int16_t>::lowest()) == sign_value::negative);
  static_assert(sign(std::int32_t{42}) == sign_value::positive);
  static_assert(sign(std::int64_t{-42}) == sign_value::negative);
}

TEST_CASE("sign of unsigned integrals", "[math]")
{
  static_assert(sign(std::uint8_t{0}) == sign_value::zero);
  static_assert(sign(std::uint8_t{1}) == sign_value::positive);
  static_assert(sign(std::numeric_limits<std::uint32_t>::max()) == sign_value::positive);
  static_assert(sign(std::uint64_t{0}) == sign_value::zero);
}

TEST_CASE("sign of floating points", "[math]")
{
  static_assert(sign(0.0) == sign_value::zero);
  static_assert(sign(-0.0) == sign_value::zero);
  static_assert(sign(0.5) == sign_value::positive);
  static_assert(sign(-0.5) == sign_value::negative);
  static_assert(sign(1.0f) == sign_value::positive);
  static_assert(sign(-1.0f) == sign_value::negative);
}

TEST_CASE("sign concepts", "[math]")
{
  static_assert(signed_type<std::int32_t>);
  static_assert(signed_type<double>);
  static_assert(!signed_type<std::uint32_t>);
  static_assert(unsigned_type<std::uint32_t>);
  static_assert(!unsigned_type<std::int32_t>);
  static_assert(!unsigned_type<double>);
}

TEST_CASE("sign values", "[math]")
{
  static_assert(sign_value::negative == -1);
  static_assert(sign_value::zero == 0);
  static_assert(sign_value::positive == 1);
}

} // namespace arcstd::math
