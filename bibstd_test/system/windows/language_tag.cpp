#include <bibocr/system/windows/language_tag.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibocr::system
{

TEST_CASE("from_language_tag", "[system]")
{
  static_assert(from_language_tag(L"de") == arcstd::util::language::german);
  CHECK(from_language_tag(L"en") == arcstd::util::language::english);
  CHECK(from_language_tag(L"en-US") == arcstd::util::language::english);
  CHECK(from_language_tag(L"de-CH") == arcstd::util::language::german);
  CHECK(from_language_tag(L"de_DE") == arcstd::util::language::german);
  CHECK(from_language_tag(L"zh-Hans-CN") == std::nullopt);
  CHECK(from_language_tag(L"") == std::nullopt);
  // Only the primary subtag names the language
  CHECK(from_language_tag(L"fr-DE") == std::nullopt);
  CHECK(from_language_tag(L"deu") == std::nullopt);
}

TEST_CASE("language_tag_map round trip", "[system]")
{
  for(const auto language : arcstd::util::enum_values<arcstd::util::language>())
  {
    CHECK(from_language_tag(language_wtag_map.at(language)) == language);
  }
}

} // namespace bibocr::system
