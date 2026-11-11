#include <arcstd/math/coordinates.hpp>
#include <arcstd/math/is_equal.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <format>
#include <stdexcept>
#include <type_traits>

namespace bibstd::math
{

TEST_CASE("coordinates construction", "[math]")
{
  static_assert(std::is_same_v<decltype(coordinates{1, 2}), coordinates<int, 2>>);
  static_assert(std::is_same_v<decltype(coordinates{1, 2, 3}), coordinates<int, 3>>);
  // The common type of the arguments wins.
  static_assert(std::is_same_v<decltype(coordinates{1, 2.0}), coordinates<double, 2>>);
  static_assert(std::is_same_v<coordinates_2d<int>, coordinates<int, 2>>);
  static_assert(std::is_same_v<coordinates_3d<int>, coordinates<int, 3>>);
  static_assert(std::is_same_v<coordinates_2d<int>::value_type, int>);

  constexpr auto from_array = coordinates<int, 3>{
    std::array{1, 2, 3}
  };
  static_assert(from_array == coordinates{1, 2, 3});

  // Values are cast to the coordinates value type.
  constexpr auto casted = coordinates<int, 2>{1.9, 2.9};
  static_assert(casted == coordinates{1, 2});
}

TEST_CASE("coordinates accessors", "[math]")
{
  constexpr auto c1 = coordinates{1};
  static_assert(c1.x() == 1);
  static_assert(c1.axis_value(0) == 1);

  constexpr auto c2 = coordinates{1, 2};
  static_assert(c2.x() == 1);
  static_assert(c2.y() == 2);
  static_assert(c2.axis_value(0) == 1);
  static_assert(c2.axis_value(1) == 2);

  constexpr auto c3 = coordinates{1, 2, 3};
  static_assert(c3.x() == 1);
  static_assert(c3.y() == 2);
  static_assert(c3.z() == 3);
  static_assert(c3.axis_value(2) == 3);

  CHECK_THROWS_AS(c3.axis_value(3), std::out_of_range);
}

TEST_CASE("coordinates operators", "[math]")
{
  static_assert(coordinates{1, 2} == coordinates{1, 2});
  static_assert(coordinates{1, 2} != coordinates{2, 1});

  static_assert(coordinates{1, 2} + coordinates{3, 4} == coordinates{4, 6});
  static_assert(coordinates{1, 2} - coordinates{3, 4} == coordinates{-2, -2});
  static_assert(coordinates{1, 2} + coordinates{0, 0} == coordinates{1, 2});
  static_assert(coordinates{1, 2} - coordinates{1, 2} == coordinates{0, 0});
  static_assert(coordinates{1, 2, 3} + coordinates{1, 1, 1} == coordinates{2, 3, 4});
  static_assert(coordinates{1, 2, 3} - coordinates{1, 1, 1} == coordinates{0, 1, 2});
}

TEST_CASE("coordinates distance", "[math]")
{
  using coords_2d = coordinates<int, 2>;
  CHECK(is_equal(coords_2d::distance(coordinates{0, 0}, coordinates{0, 0}), 0.0));
  CHECK(is_equal(coords_2d::distance(coordinates{0, 0}, coordinates{3, 4}), 5.0, 1e-9));
  // Distance is symmetric.
  CHECK(is_equal(coords_2d::distance(coordinates{3, 4}, coordinates{0, 0}), 5.0, 1e-9));
  CHECK(is_equal(coords_2d::distance(coordinates{-3, -4}, coordinates{0, 0}), 5.0, 1e-9));

  using coords_3d = coordinates<int, 3>;
  CHECK(is_equal(coords_3d::distance(coordinates{0, 0, 0}, coordinates{2, 3, 6}), 7.0, 1e-9));

  using coords_2d_double = coordinates<double, 2>;
  CHECK(is_equal(coords_2d_double::distance(coordinates{0.0, 0.0}, coordinates{0.0, 0.5}), 0.5, 1e-9));
}

TEST_CASE("coordinates formatter", "[math]")
{
  CHECK(std::format("{}", coordinates{1, 2}) == "(1,2)");
  CHECK(std::format("{}", coordinates{1, 2, 3}) == "(1,2,3)");
  CHECK(std::format("{}", coordinates{-1, -2}) == "(-1,-2)");
}

} // namespace bibstd::math
