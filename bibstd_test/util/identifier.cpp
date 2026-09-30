#include <bibstd/util/exception.hpp>
#include <bibstd/util/identifier.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibstd::util
{

TEST_CASE("identifier", "[util]")
{
  CHECK(identifier{"bible_server2"}.string() == "bible_server2");
  CHECK(identifier{"a"} < identifier{"b"});
  CHECK(identifier{"bible server"} == identifier{"bible_server"});
  CHECK(identifier{"bible.server"} == identifier{"bible_server"});
  CHECK(identifier{"bible-server"} == identifier{"bible-server"});
  CHECK(identifier{"../bible"} == identifier{"___bible"});
  CHECK_THROWS_AS(identifier{""}, util::exception);
  // From outside, without an exception
  CHECK(identifier::from("bible server") == identifier{"bible_server"});
  CHECK_FALSE(identifier::from(""));
}

} // namespace bibstd::util
