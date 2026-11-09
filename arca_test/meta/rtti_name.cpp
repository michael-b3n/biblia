#include <arca/meta/rtti_name.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibstd::math
{
namespace test
{

///
/// Nested struct for testing rtti_name with nested types.
///
struct nested_test_struct final
{
  struct more_nested_test_struct final
  {};
};

} // namespace test

TEST_CASE("rtti_name", "[meta]")
{
  struct test_struct final
  {};

  static_assert(meta::rtti_name<int>::value == "int");
  static_assert(meta::rtti_name<test::nested_test_struct>::value == "nested_test_struct");
  static_assert(meta::rtti_name<test::nested_test_struct::more_nested_test_struct>::value == "more_nested_test_struct");
  static_assert(meta::rtti_name<test_struct>::value == "test_struct");
  // Templated types are not supported by rtti_name, so this should not compile:
  // static_assert(meta::rtti_name<nested_templated_test_struct<int>>::value == "nested_templated_test_struct<int>");
}

} // namespace bibstd::math
