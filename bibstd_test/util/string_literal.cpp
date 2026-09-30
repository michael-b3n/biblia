#include <bibstd/util/string_literal.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibstd::util
{
namespace
{

template<string_literal Literal>
constexpr auto view_of = Literal.view();

} // namespace

TEST_CASE("string_literal_keeps_the_characters", "[util]")
{
  STATIC_REQUIRE(string_literal{"key"}.view() == "key");
  STATIC_REQUIRE(string_literal{""}.view().empty());
  STATIC_REQUIRE(view_of<"name"> == "name");
}

TEST_CASE("string_literal_takes_only_one_terminating_zero", "[util]")
{
  STATIC_REQUIRE(string_literal<4>::terminated_once("key"));
  STATIC_REQUIRE(string_literal<1>::terminated_once(""));
  // Would not compile as template argument
  STATIC_REQUIRE_FALSE(string_literal<4>::terminated_once("a\0b"));
  STATIC_REQUIRE_FALSE(string_literal<3>::terminated_once("a\0"));
  static constexpr char unterminated[] = {'a', 'b'};
  STATIC_REQUIRE_FALSE(string_literal<2>::terminated_once(unterminated));
}

} // namespace bibstd::util
