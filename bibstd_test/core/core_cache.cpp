#include "test_utils/files.hpp"
#include "test_utils/temp_folder.hpp"

#include <bibstd/core/core_cache.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace bibstd::core
{

TEST_CASE("core_cache_keeps_values", "[core]")
{
  const auto folder = test_utils::temp_folder{"core_cache_keeps_values"};
  // Its folder is created as well
  const auto file = folder.path() / "sub" / "test.sqlite";
  const auto binary = std::string{"zero\0inside", 11};
  const auto large = std::string(1'000'000, 'x');
  {
    auto cache = core_cache{file};
    CHECK_FALSE(cache.get("a"));
    cache.set("a", "0");
    cache.set("a", "1");
    cache.set("b", "2");
    cache.set("b", std::nullopt);
    cache.set("missing", std::nullopt);
    cache.set("c", "line\nbreak\ttab\\slash 'quote' \xC3\xA4");
    cache.set("key\nwith\tbreaks 'quote'", "x");
    cache.set("empty", "");
    cache.set("", "empty key");
    cache.set(binary, binary);
    cache.set("large", large);
    CHECK(cache.get("a") == "1");
    CHECK_FALSE(cache.get("b"));
  }
  // Beyond the cache that set them
  const auto cache = core_cache{file};
  CHECK(cache.get("a") == "1");
  CHECK_FALSE(cache.get("b"));
  CHECK_FALSE(cache.get("missing"));
  CHECK(cache.get("c") == "line\nbreak\ttab\\slash 'quote' \xC3\xA4");
  CHECK(cache.get("key\nwith\tbreaks 'quote'") == "x");
  CHECK(cache.get("empty") == "");
  CHECK(cache.get("") == "empty key");
  CHECK(cache.get(binary) == binary);
  CHECK_FALSE(cache.get("zero"));
  CHECK(cache.get("large") == large);
}

TEST_CASE("core_cache_shares_its_file", "[core]")
{
  const auto folder = test_utils::temp_folder{"core_cache_shares_its_file"};
  const auto file = folder.path() / "test.sqlite";
  // Two at once, as two processes have them
  auto first = core_cache{file};
  auto second = core_cache{file};
  first.set("a", "1");
  CHECK(second.get("a") == "1");
  second.set("a", "2");
  second.set("b", "3");
  CHECK(first.get("a") == "2");
  CHECK(first.get("b") == "3");
  first.set("b", std::nullopt);
  CHECK_FALSE(second.get("b"));
}

TEST_CASE("core_cache_of_a_broken_file_holds_nothing", "[core]")
{
  const auto folder = test_utils::temp_folder{"core_cache_of_a_broken_file_holds_nothing"};
  const auto file = folder.path() / "test.sqlite";
  const auto content = std::string{"+a\t1\nno database, e.g. a cache file of a former version\n"};
  test_utils::write_file(file, content);
  {
    auto cache = core_cache{file};
    CHECK_FALSE(cache.get("a"));
    cache.set("a", "1");
    CHECK_FALSE(cache.get("a"));
  }
  // Left as it is
  CHECK(test_utils::read_file(file) == content);
}

} // namespace bibstd::core
