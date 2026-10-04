#include <bibstd/system/windows/language_tag.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibstd::system
{

TEST_CASE("from_language_tag", "[system]")
{
  static_assert(from_language_tag(L"de") == util::language::german);
  CHECK(from_language_tag(L"en") == util::language::english);
  CHECK(from_language_tag(L"en-US") == util::language::english);
  CHECK(from_language_tag(L"de-CH") == util::language::german);
  CHECK(from_language_tag(L"de_DE") == util::language::german);
  CHECK(from_language_tag(L"zh-Hans-CN") == std::nullopt);
  CHECK(from_language_tag(L"") == std::nullopt);
  // Only the primary subtag names the language
  CHECK(from_language_tag(L"fr-DE") == std::nullopt);
  CHECK(from_language_tag(L"deu") == std::nullopt);
}

TEST_CASE("language_tag_map round trip", "[system]")
{
  for(const auto language : util::enum_values<util::language>())
  {
    CHECK(from_language_tag(language_wtag_map.at(language)) == language);
  }
}

} // namespace bibstd::system
