#include <arca/util/format.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <list>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace bibstd::util
{

TEST_CASE("format join", "[util]")
{
  CHECK(format::join(std::vector<int>{}, ",").empty());
  CHECK(format::join(std::vector{1}, ",") == "1");
  CHECK(format::join(std::vector{1, 2, 3}, ",") == "1,2,3");
  CHECK(format::join(std::vector{1, 2, 3}, "") == "123");
  CHECK(format::join(std::vector{1, 2, 3}, " -> ") == "1 -> 2 -> 3");
  CHECK(format::join(std::array{"a", "b"}, ",") == "a,b");
  CHECK(format::join(std::list<std::string>{"a", "b"}, ";") == "a;b");
  CHECK(format::join(std::vector{1.5, 2.5}, ",") == "1.5,2.5");
}

TEST_CASE("format join on views", "[util]")
{
  // join takes its range by const reference, so only const iterable views work.
  const auto values = std::vector{1, 2, 3, 4};
  CHECK(format::join(values | std::views::take(2), ",") == "1,2");
  CHECK(format::join(values | std::views::drop(2), ",") == "3,4");
  CHECK(format::join(values | std::views::reverse, ",") == "4,3,2,1");
}

TEST_CASE("format to_string of optional", "[util]")
{
  CHECK(format::to_string(std::optional<int>{}) == "nullopt");
  CHECK(format::to_string(std::optional<int>{1}) == "1");
  CHECK(format::to_string(std::optional<double>{1.5}) == "1.5");
  CHECK(format::to_string(std::optional<std::string>{"a"}) == "a");
  CHECK(format::to_string(std::optional<bool>{true}) == "true");
}

} // namespace bibstd::util
