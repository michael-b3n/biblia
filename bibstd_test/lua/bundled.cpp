#include "test_utils/script_settings.hpp"
#include "test_utils/temp_folder.hpp"

#include <bibstd/util/identifier.hpp>
#include <bibstd/workflow/workflow_bible_ref_lookup.hpp>
#include <bibstd/workflow/workflow_script.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::lua
{
namespace
{

using lookup = workflow::workflow_bible_ref_lookup;

///
/// Script workflow with the bundled scripts, the scripts of the user are not loaded.
///
struct bundled_fixture final
{
  test_utils::temp_folder folder;
  std::shared_ptr<workflow::workflow_script> script;

  explicit bundled_fixture(std::string_view name);

  ///
  /// \return the url the script \p id builds for the verses of the chapter, none if it builds none
  ///
  [[nodiscard]] auto url(
    std::string_view id,
    const std::vector<std::string>& translations,
    const std::string& book,
    std::int64_t chapter,
    std::int64_t verse_begin,
    std::int64_t verse_end
  ) const -> std::optional<std::string>;
};

///
///
bundled_fixture::bundled_fixture(const std::string_view name)
  : folder{name}
  , script{std::make_shared<workflow::workflow_script>(test_utils::make_script_settings(folder.path()))}
{
  test_utils::load_scripts(*script);
}

///
///
auto bundled_fixture::url(
  const std::string_view id,
  const std::vector<std::string>& translations,
  const std::string& book,
  const std::int64_t chapter,
  const std::int64_t verse_begin,
  const std::int64_t verse_end
) const -> std::optional<std::string>
{
  const auto output =
    script->run<lookup::url_manifest>(util::identifier{id}, {translations, book, chapter, verse_begin, verse_end});
  return output ? output->get<"url">() : std::nullopt;
}

} // namespace

TEST_CASE("bundled_lookup_bibleserver_offers_its_translations", "[lua]")
{
  using names_type = std::vector<std::string>;
  const auto fixture = bundled_fixture{"bundled_lookup_bibleserver_offers_its_translations"};
  const auto id = util::identifier{"lookup_bibleserver"};

  const auto german = fixture.script->run<lookup::translations_manifest>(id, {"german"});
  REQUIRE(german);
  CHECK(german->get<"names">().size() == 17);
  CHECK(german->get<"defaults">() == names_type{"NGÜ", "ELB"});
  const auto english = fixture.script->run<lookup::translations_manifest>(id, {"english"});
  REQUIRE(english);
  CHECK(english->get<"defaults">() == names_type{"NIV", "ESV"});
  // No defaults for a language the script does not know
  const auto other = fixture.script->run<lookup::translations_manifest>(id, {"french"});
  REQUIRE(other);
  CHECK(other->get<"names">().size() == 17);
  CHECK_FALSE(other->get<"defaults">());
}

TEST_CASE("bundled_lookup_bibleserver_builds_the_urls", "[lua]")
{
  const auto fixture = bundled_fixture{"bundled_lookup_bibleserver_builds_the_urls"};
  const auto id = std::string_view{"lookup_bibleserver"};
  const auto site = std::string{"https://www.bibleserver.com/"};

  // Translations side by side in the order chosen, umlauts encoded
  CHECK(fixture.url(id, {"NGÜ", "ELB"}, "john", 3, 16, 16) == site + "NG%C3%9C.ELB/Johannes3%2C16");
  CHECK(fixture.url(id, {"ELB", "NGÜ"}, "john", 3, 16, 18) == site + "ELB.NG%C3%9C/Johannes3%2C16-18");
  CHECK(fixture.url(id, {"NeÜ"}, "kings1", 2, 3, 3) == site + "Ne%C3%9C/1.K%C3%B6nige2%2C3");
  CHECK(fixture.url(id, {"ESV"}, "psalms", 23, 1, 6) == site + "ESV/Psalm23%2C1-6");
  // A translation the script does not offer is left out
  CHECK(fixture.url(id, {"XXX", "LUT"}, "john", 3, 16, 16) == site + "LUT/Johannes3%2C16");
  // No url without a translation or a book
  CHECK_FALSE(fixture.url(id, {}, "john", 3, 16, 16));
  CHECK_FALSE(fixture.url(id, {"XXX"}, "john", 3, 16, 16));
  CHECK_FALSE(fixture.url(id, {"LUT"}, "tobit", 1, 1, 1));
}

TEST_CASE("bundled_lookup_bible_com_offers_its_translations", "[lua]")
{
  using names_type = std::vector<std::string>;
  const auto fixture = bundled_fixture{"bundled_lookup_bible_com_offers_its_translations"};
  const auto id = util::identifier{"lookup_bible_com"};

  const auto german = fixture.script->run<lookup::translations_manifest>(id, {"german"});
  REQUIRE(german);
  CHECK(german->get<"names">().size() == 12);
  CHECK(german->get<"defaults">() == names_type{"ELB"});
  const auto english = fixture.script->run<lookup::translations_manifest>(id, {"english"});
  REQUIRE(english);
  CHECK(english->get<"defaults">() == names_type{"NIV"});
}

TEST_CASE("bundled_lookup_bible_com_builds_the_urls", "[lua]")
{
  const auto fixture = bundled_fixture{"bundled_lookup_bible_com_builds_the_urls"};
  const auto id = std::string_view{"lookup_bible_com"};
  const auto site = std::string{"https://www.bible.com/bible/"};

  // The version of the translation, the code of the book
  CHECK(fixture.url(id, {"NIV"}, "john", 3, 16, 16) == site + "111/JHN.3.16.NIV");
  CHECK(fixture.url(id, {"ELB"}, "kings1", 2, 3, 5) == site + "57/1KI.2.3-5.ELB");
  // One translation a page, the first one chosen that the script offers
  CHECK(fixture.url(id, {"SCH2000", "KJV"}, "psalms", 23, 1, 1) == site + "157/PSA.23.1.SCH2000");
  CHECK(fixture.url(id, {"XXX", "KJV"}, "psalms", 23, 1, 1) == site + "1/PSA.23.1.KJV");
  // No url without a translation or a book
  CHECK_FALSE(fixture.url(id, {}, "john", 3, 16, 16));
  CHECK_FALSE(fixture.url(id, {"NIV"}, "tobit", 1, 1, 1));
}

} // namespace bibstd::lua
