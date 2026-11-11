#include <arcstd/meta/chrono.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>

namespace bibstd::meta
{

TEST_CASE("is_duration", "[meta]")
{
  static_assert(is_duration_v<std::chrono::seconds>);
  static_assert(is_duration_v<std::chrono::milliseconds>);
  static_assert(is_duration_v<std::chrono::nanoseconds>);
  static_assert(is_duration_v<std::chrono::duration<double>>);
  static_assert(is_duration_v<std::chrono::duration<int, std::ratio<1, 3>>>);
  static_assert(is_duration_v<const std::chrono::seconds>);
  static_assert(is_duration_v<std::chrono::seconds&>);

  static_assert(!is_duration_v<int>);
  static_assert(!is_duration_v<std::chrono::system_clock::time_point>);
}

} // namespace bibstd::meta
