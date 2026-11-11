#include <arcstd/meta/convert_if.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

namespace bibstd::meta
{

TEST_CASE("convert_if", "[meta]")
{
  static_assert(std::is_same_v<convert_if_t<true, int, double>, double>);
  static_assert(std::is_same_v<convert_if_t<false, int, double>, int>);
  static_assert(std::is_same_v<convert_if_t<true, int, int>, int>);
  static_assert(std::is_same_v<convert_if<true, int, double>::type, double>);
  static_assert(std::is_same_v<convert_if<false, int, double>::type, int>);

  static_assert(std::is_same_v<convert_if_t<std::is_integral_v<int>, int, double>, double>);
  static_assert(std::is_same_v<convert_if_t<std::is_integral_v<float>, float, double>, float>);
}

} // namespace bibstd::meta
