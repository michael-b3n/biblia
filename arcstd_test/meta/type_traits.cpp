#include <arcstd/meta/type_traits.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

namespace arcstd::meta
{

struct type_traits_test_struct final
{};

TEST_CASE("is_templated", "[meta]")
{
  static_assert(is_templated_v<std::vector<int>>);
  static_assert(is_templated_v<std::optional<int>>);
  static_assert(is_templated_v<std::tuple<int, double>>);
  static_assert(!is_templated_v<int>);
  static_assert(!is_templated_v<type_traits_test_struct>);

  // Cv-qualifiers and references are stripped before the check.
  static_assert(is_templated_v<const std::vector<int>&>);
  static_assert(is_templated_v<std::vector<int>&&>);
  static_assert(!is_templated_v<const int&>);
}

TEST_CASE("remove_wrapper", "[meta]")
{
  static_assert(std::is_same_v<remove_wrapper_t<std::optional<int>>, int>);
  // Defaulted template arguments, such as the allocator of std::vector, are ignored.
  static_assert(std::is_same_v<remove_wrapper_t<std::vector<int>>, int>);
  static_assert(std::is_same_v<remove_wrapper_t<std::optional<std::optional<int>>>, std::optional<int>>);
  // Non wrapped types are returned unchanged.
  static_assert(std::is_same_v<remove_wrapper_t<int>, int>);
  static_assert(std::is_same_v<remove_wrapper_t<type_traits_test_struct>, type_traits_test_struct>);
  // Decay is applied first.
  static_assert(std::is_same_v<remove_wrapper_t<const std::optional<int>&>, int>);
  static_assert(std::is_same_v<remove_wrapper_t<const int&>, const int&>);
}

TEST_CASE("are_same", "[meta]")
{
  static_assert(are_same_v<>);
  static_assert(are_same_v<int>);
  static_assert(are_same_v<int, int>);
  static_assert(are_same_v<int, int, int>);
  static_assert(!are_same_v<int, double>);
  static_assert(!are_same_v<int, int, double>);
  static_assert(!are_same_v<int, double, int>);
  // Cv-qualifiers and references make types distinct.
  static_assert(!are_same_v<int, const int>);
  static_assert(!are_same_v<int, int&>);
}

TEST_CASE("conditional_unsigned", "[meta]")
{
  static_assert(std::is_same_v<conditional_unsigned_t<std::int8_t>, std::uint8_t>);
  static_assert(std::is_same_v<conditional_unsigned_t<std::int32_t>, std::uint32_t>);
  static_assert(std::is_same_v<conditional_unsigned_t<std::uint32_t>, std::uint32_t>);
  static_assert(std::is_same_v<conditional_unsigned_t<std::int64_t>, std::uint64_t>);
  // Floating point types are left untouched.
  static_assert(std::is_same_v<conditional_unsigned_t<float>, float>);
  static_assert(std::is_same_v<conditional_unsigned_t<double>, double>);
  // Const and volatile indicators are removed.
  static_assert(std::is_same_v<conditional_unsigned_t<const std::int32_t>, const std::uint32_t>);
  static_assert(std::is_same_v<conditional_unsigned_t<volatile std::int32_t>, volatile std::uint32_t>);
  static_assert(std::is_same_v<conditional_unsigned_t<const double>, const double>);
}

} // namespace arcstd::meta
