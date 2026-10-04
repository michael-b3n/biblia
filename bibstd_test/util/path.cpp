#include <bibstd/util/path.hpp>

#include <catch2/catch_test_macros.hpp>

#include <ranges>
#include <string>
#include <vector>

namespace bibstd::util
{

TEST_CASE("path", "[util]")
{
  CHECK(path{"ocr"}.sections() == std::vector<std::string>{"ocr"});
  CHECK(path{"ocr.engine.language"}.sections() == std::vector<std::string>{"ocr", "engine", "language"});
  CHECK(path{std::string{"a.b"}}.string() == "a.b");
  // Empty sections are left out, so a malformed path is read like the path it was meant to be
  CHECK(path{".ocr..language."} == path{"ocr.language"});
  CHECK(path{""}.empty());
  // From sections, each read like a path
  CHECK(
    path{
      std::vector<std::string>{"a", "b"}
  } == path{"a.b"}
  );
  CHECK(
    path{
      std::vector<std::string>{"a.b", "", "c"}
  } == path{"a.b.c"}
  );
  CHECK(path{path{"a.b.c"}.sections() | std::views::take(2)} == path{"a.b"});
  CHECK(path{std::vector<std::string>{}}.empty());
  CHECK(path{"a.b"}.contains(path{"a.b.c"}));
  CHECK(path{"a.b"}.contains(path{"a.b"}));
  CHECK_FALSE(path{"a.b"}.contains(path{"a"}));
  CHECK_FALSE(path{"a.b"}.contains(path{"a.bc"}));
}

} // namespace bibstd::util
