#include <arcstd/math/valid_floating_point.hpp>

#include <catch2/catch_test_macros.hpp>

#include <limits>

namespace bibstd::math
{

// Constants
template<typename T>
constexpr auto nan = std::numeric_limits<T>::quiet_NaN();
template<typename T>
constexpr auto signaling_nan = std::numeric_limits<T>::signaling_NaN();
template<typename T>
constexpr auto inf = std::numeric_limits<T>::infinity();

TEST_CASE("is_valid single value", "[math]")
{
  CHECK(is_valid(0.0));
  CHECK(is_valid(-0.0));
  CHECK(is_valid(1.5));
  CHECK(is_valid(std::numeric_limits<double>::max()));
  CHECK(is_valid(std::numeric_limits<double>::lowest()));
  CHECK(is_valid(std::numeric_limits<double>::denorm_min()));
  CHECK(!is_valid(nan<double>));
  CHECK(!is_valid(signaling_nan<double>));
  CHECK(!is_valid(inf<double>));
  CHECK(!is_valid(-inf<double>));

  CHECK(is_valid(0.0f));
  CHECK(!is_valid(nan<float>));
  CHECK(!is_valid(inf<float>));
}

TEST_CASE("is_valid multiple values", "[math]")
{
  CHECK(is_valid(0.0, 1.0, 2.0));
  CHECK(!is_valid(0.0, nan<double>, 2.0));
  CHECK(!is_valid(nan<double>, 1.0, 2.0));
  CHECK(!is_valid(0.0, 1.0, inf<double>));
  CHECK(!is_valid(inf<double>, nan<double>));

  // An empty pack folds to true.
  CHECK(is_valid());

  // Mixed floating point types are supported.
  CHECK(is_valid(0.0, 1.0f));
  CHECK(!is_valid(0.0, nan<float>));
}

} // namespace bibstd::math
