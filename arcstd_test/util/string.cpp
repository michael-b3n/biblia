#include <arcstd/util/string.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibstd::util
{

TEST_CASE("split", "[util]")
{
  static_assert(string::split("", ",").empty());
  static_assert(string::split("a,b,c", ",") == std::vector<std::string>{"a", "b", "c"});
  static_assert(string::split("a,,c", ",") == std::vector<std::string>{"a", "", "c"});
  static_assert(string::split("abc", ",") == std::vector<std::string>{"abc"});
  static_assert(string::split("a,b,c,", ",") == std::vector<std::string>{"a", "b", "c", ""});
  static_assert(string::split(",a,b,c", ",") == std::vector<std::string>{"", "a", "b", "c"});
  static_assert(string::split("a|b|c", "|") == std::vector<std::string>{"a", "b", "c"});
  static_assert(string::split("a|b|c|d|e", "|") == std::vector<std::string>{"a", "b", "c", "d", "e"});
}

TEST_CASE("join", "[util]")
{
  CHECK(string::join(std::array{"a", "b", "c"}, ",") == std::string_view{"a,b,c"});
  CHECK(string::join(std::array{"a", "", "c"}, ",") == std::string_view{"a,,c"});
  CHECK(string::join(std::array{"abc"}, ",") == std::string_view{"abc"});
  CHECK(string::join(std::array{"a", "b", "c", ""}, ",") == std::string_view{"a,b,c,"});
  CHECK(string::join(std::array{"", "a", "b", "c"}, ",") == std::string_view{",a,b,c"});
  CHECK(string::join(std::array{"a", "b", "c"}, "|") == std::string_view{"a|b|c"});
  CHECK(string::join(std::array{"a", "b", "c", "d", "e"}, "|") == std::string_view{"a|b|c|d|e"});
}

TEST_CASE("starts_with", "[util]")
{
  static_assert(string::starts_with("Hello, World!", "Hello"));
  static_assert(!string::starts_with("Hello, World!", "World"));
  static_assert(string::starts_with("Test", "T"));
  static_assert(!string::starts_with("Test", "t"));
  static_assert(string::starts_with("Test", 'T'));
  static_assert(!string::starts_with("Test", 't'));
  static_assert(!string::starts_with("", 'a'));
  static_assert(!string::starts_with("", "any"));
  CHECK_THROWS(string::starts_with("Example", ""));
}

TEST_CASE("ends_with", "[util]")
{
  static_assert(string::ends_with("Hello, World!", "World!"));
  static_assert(!string::ends_with("Hello, World!", "Hello"));
  static_assert(string::ends_with("Test", "t"));
  static_assert(!string::ends_with("Test", "T"));
  static_assert(string::ends_with("Test", 't'));
  static_assert(!string::ends_with("Test", 'T'));
  static_assert(!string::ends_with("", 'a'));
  static_assert(!string::ends_with("", "any"));
  CHECK_THROWS(string::ends_with("Example", ""));
}

TEST_CASE("ends_with_formatted_uint", "[util]")
{
  static_assert(string::ends_with_formatted_uint("", " ({})") == std::nullopt);
  static_assert(string::ends_with_formatted_uint("", "") == std::nullopt);
  static_assert(string::ends_with_formatted_uint("Test (1)", " ({})") == 1);
  static_assert(string::ends_with_formatted_uint("Example [42]", " [{}]") == 42);
  static_assert(string::ends_with_formatted_uint("No number here", " ({})") == std::nullopt);
  static_assert(string::ends_with_formatted_uint("Invalid format (abc)", " ({})") == std::nullopt);
  static_assert(string::ends_with_formatted_uint("Multiple numbers (1) and (2)", " ({})") == 2);
  static_assert(string::ends_with_formatted_uint("Number at the end 123", " {}") == 123);
  static_assert(string::ends_with_formatted_uint("Empty format", "") == std::nullopt);
  static_assert(string::ends_with_formatted_uint("Format without placeholder", " ()") == std::nullopt);
}

} // namespace bibstd::util
