#include <arca/util/contains.hpp>
#include <bibstd/bible/ocr_book_variants.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace bibstd::bible
{

TEST_CASE("ocr_book_variants name_variant_aliases", "[bible]")
{
  GIVEN("name variant with several misreadable chars")
  {
    // '1' can be misread as 'I' and 'l', 'M' as 'm' and 'o' as '0' and 'O'.
    const auto aliases = ocr_book_variants::name_variant_aliases("1Mose", util::language::german);
    CHECK(util::contains(aliases, std::string{"IMose"}));
    CHECK(util::contains(aliases, std::string{"1mose"}));
    CHECK(util::contains(aliases, std::string{"1MOse"}));
    CHECK(util::contains(aliases, std::string{"lm0se"}));
    CHECK(util::contains(aliases, std::string{"lm0Se"}));
    CHECK_FALSE(util::contains(aliases, std::string{"1Mose"}));
  }
  GIVEN("name variant without misreadable chars")
  {
    CHECK(ocr_book_variants::name_variant_aliases("Ri", util::language::german).empty());
  }
  GIVEN("name variant with multibyte chars")
  {
    const auto aliases = ocr_book_variants::name_variant_aliases("1Kön", util::language::german);
    CHECK(aliases.size() == 2);
    CHECK(util::contains(aliases, std::string{"IKön"}));
    CHECK(util::contains(aliases, std::string{"lKön"}));
  }
  GIVEN("empty name variant")
  {
    CHECK(ocr_book_variants::name_variant_aliases("", util::language::german).empty());
  }
}

TEST_CASE("ocr_book_variants name_variants_with_aliases", "[bible]")
{
  GIVEN("all german name variants")
  {
    const auto variants = ocr_book_variants::name_variants_with_aliases(util::language::german);
    const auto contains = [&](const book_id id, const std::string_view name_variant)
    { return util::contains(variants, [&](const auto& e) { return e.first == id && e.second == name_variant; }); };
    CHECK(contains(book_id::genesis, "1Mose"));
    CHECK(contains(book_id::genesis, "lM0se"));
    CHECK(contains(book_id::revelation, "Offenbarung"));
  }
}

} // namespace bibstd::bible
