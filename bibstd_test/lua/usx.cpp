#include "test_utils/scripture_store.hpp"
#include "test_utils/zip_archive.hpp"

#include <bibstd/bible/common.hpp>
#include <bibstd/bible/reference.hpp>
#include <bibstd/bible/scripture.hpp>
#include <bibstd/bible/scripture_types.hpp>
#include <bibstd/util/enum.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/interfaces/catch_interfaces_capture.hpp>
#include <pugixml.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace bibstd::lua
{
namespace
{

using bible::book_id;
using test_utils::read_scripture;
using test_utils::verse_of;

///
/// \return the entries of a USX bundle with the metadata of a scripture named "Test", a book document per book
///
[[nodiscard]] auto usx_entries() -> std::vector<io::test_utils::zip_entry>
{
  auto entries = std::vector<io::test_utils::zip_entry>{
    {        "bundle/metadata.xml",
     R"(<?xml version="1.0"?><DBLMetadata><identification><name>Test</name><nameLocal>Test lokal</nameLocal>)"
     R"(<abbreviation>TST</abbreviation></identification><language><name>German</name></language>)"
     R"(<copyright><fullStatement><statementContent type="xhtml"><p>Public</p> <p>Domain</p></statementContent>)"
     R"(</fullStatement></copyright></DBLMetadata>)"                                                               },
    // A reduced copy deeper in the bundle, the one in its root counts
    {"bundle/release/metadata.xml", "<DBLMetadata><identification><name>Copy</name></identification></DBLMetadata>"},
  };
  for(const auto code : test_utils::usfm_codes)
  {
    entries.emplace_back(
      std::format("bundle/release/USX_1/{}.usx", code),
      std::format(
        R"(<?xml version="1.0" encoding="utf-8"?><usx version="3.0"><book code="{0}" style="id"/>)"
        R"(<para style="h">{0} short</para><para style="toc1">{0} long</para><chapter number="1" style="c" sid="{0} 1"/>)"
        R"(<para style="s1">Heading</para><para style="p"><verse number="1" style="v" sid="{0} 1:1"/>The <char )"
        R"(style="wj">word &amp; <char style="nd">Lord</char></char><note caller="+" style="f"><char style="ft">Note)"
        R"(</char></note><verse eid="{0} 1:1"/><verse number="2" style="v" sid="{0} 1:2"/>Next<verse eid="{0} 1:2"/>)"
        R"(</para><chapter eid="{0} 1"/></usx>)",
        code
      )
    );
  }
  return entries;
}

///
/// A scripture of the local test scriptures and the file it was read from.
///
struct shipped_scripture final
{
  std::string bundle;
  std::shared_ptr<const bible::scripture> loaded;
};

///
/// Reading a bundle takes a while, so they are read once and shared by the test cases.
/// \return the local test scriptures, the test is skipped without any
///
[[nodiscard]] auto shipped_scriptures() -> const std::vector<shipped_scripture>&
{
  static const auto scriptures = []()
  {
    auto result = std::vector<shipped_scripture>{};
    auto error = std::error_code{};
    const auto is_zip = [](const auto& file) { return file.path().extension() == ".zip"; };
    for(const auto& file : std::filesystem::directory_iterator{BIBSTD_TEST_SCRIPTURE_DIR, error} | std::views::filter(is_zip))
    {
      // Each test case runs in its own process, which reads them into its own folder
      const auto fixture =
        test_utils::scripture_store{Catch::getResultCapture().getCurrentTestName(), {}, {file.path()}};
      const auto loaded = fixture.store->scriptures();
      REQUIRE(loaded.size() == 1);
      result.emplace_back(file.path().filename().string(), loaded.begin()->second);
    }
    return result;
  }();
  if(scriptures.empty())
  {
    // The scriptures are not part of the repository, \see bibstd_test/res/scripture/README.md.
    SKIP(std::format("no scriptures in {}", BIBSTD_TEST_SCRIPTURE_DIR));
  }
  return scriptures;
}

///
/// \return the local test scripture of \p bundle, nullptr if there is none
///
[[nodiscard]] auto find_bundle(const std::string_view bundle) -> std::shared_ptr<const bible::scripture>
{
  const auto& scriptures = shipped_scriptures();
  const auto found = std::ranges::find(scriptures, bundle, &shipped_scripture::bundle);
  return found != std::ranges::cend(scriptures) ? found->loaded : nullptr;
}

///
/// \return the passage markup a paragraph section of the given position opens with
///
[[nodiscard]] auto paragraph_open(const std::string_view position) -> std::string
{
  using markup = bible::passage_markup;
  return std::format(R"(<{} {}="{}">)", markup::paragraph, markup::paragraph_attribute, position);
}

} // namespace

TEST_CASE("usx_reads_a_bundle", "[lua]")
{
  const auto scripture = read_scripture("usx_reads_a_bundle", usx_entries());
  REQUIRE(scripture);
  const auto& info = scripture->information();
  CHECK(info.name == "Test lokal");
  CHECK(info.abbreviation == "TST");
  CHECK(info.language == "German");
  CHECK(info.copyright == "PublicDomain");
  // End milestones end nothing, a note is no text
  CHECK(verse_of(*scripture, book_id::john, 1, 1) == R"(<p data-id="begin">The word &amp; <i>Lord</i></p>)");
  CHECK(verse_of(*scripture, book_id::john, 1, 2) == R"(<p data-id="continue">Next</p>)");
  const auto names = scripture->book_information(book_id::john);
  REQUIRE(names);
  CHECK(names->short_name == "JHN short");
  CHECK(names->long_name == "JHN long");
}

TEST_CASE("usx_reads_an_archive_of_usx_files", "[lua]")
{
  // Without metadata it is named after its file, like an archive of USFM files
  auto entries = usx_entries();
  std::erase_if(entries, [](const auto& entry) { return entry.first.ends_with("metadata.xml"); });
  const auto scripture = read_scripture("usx_reads_an_archive_of_usx_files", entries);
  REQUIRE(scripture);
  CHECK(scripture->information().name == "scripture");
  CHECK(verse_of(*scripture, book_id::john, 1, 2) == R"(<p data-id="continue">Next</p>)");
}

TEST_CASE("usx_provides_names_for_every_book", "[lua]")
{
  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    auto short_names = std::vector<std::string>{};
    for(const auto book : util::enum_values<book_id>())
    {
      INFO(std::format("book: {}", util::enum_name(book)));
      const auto names = loaded->book_information(book);
      REQUIRE(names.has_value());
      // Every form is filled from the header paragraphs of the book, falling back to the closest alternative
      CHECK(!names->short_name.empty());
      CHECK(!names->abbreviation.empty());
      CHECK(!names->long_name.empty());
      CHECK(names->short_name != util::enum_name(book));
      // The header block ends at the first chapter, names grown into verses would be longer
      CHECK(names->long_name.size() < 100u);
      CHECK(names->short_name.size() <= names->long_name.size());
      short_names.push_back(names->short_name);
    }
    // A name shared by two books would be something other than their own header
    std::ranges::sort(short_names);
    CHECK(std::ranges::adjacent_find(short_names) == std::ranges::cend(short_names));
  }
}

TEST_CASE("usx_book_names_are_read_from_the_book_documents", "[lua]")
{
  SECTION("german scripture")
  {
    const auto loaded = find_bundle("text-542b32484b6e38c2-246437.zip");
    REQUIRE(loaded != nullptr);
    const auto genesis = loaded->book_information(book_id::genesis);
    REQUIRE(genesis.has_value());
    CHECK(genesis->short_name == "1. Mose");
    CHECK(genesis->long_name == "Das 1. Buch Mose (Genesis)");
  }

  SECTION("english scripture")
  {
    const auto loaded = find_bundle("text-de4e12af7f28f599-245514.zip");
    REQUIRE(loaded != nullptr);
    const auto genesis = loaded->book_information(book_id::genesis);
    REQUIRE(genesis.has_value());
    CHECK(genesis->short_name == "Genesis");
    CHECK(genesis->abbreviation == "Gen");
    CHECK(genesis->long_name == "The First Book of Moses, called Genesis");
  }

  SECTION("scripture without running header paragraphs")
  {
    // This bundle ships no "h" paragraph at all, the table of contents entries carry the names
    const auto loaded = find_bundle("text-f492a38d0e52db0f-258505.zip");
    REQUIRE(loaded != nullptr);
    const auto song = loaded->book_information(book_id::song_of_solomon);
    REQUIRE(song.has_value());
    CHECK(song->short_name == "Hohelied");
    CHECK(song->abbreviation == "Hld.");
  }
}

TEST_CASE("usx_reads_its_information_from_the_bundle_root_metadata", "[lua]")
{
  // The bundles carry a reduced "metadata.xml" in their "release" folder too
  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    const auto& info = loaded->information();
    CHECK(info.name != "Unknown Scripture");
    CHECK(info.abbreviation != "Unknown Abbreviation");
    CHECK(!info.language.empty());
    REQUIRE(info.copyright.has_value());
    CHECK(!info.copyright->empty());
  }
}

TEST_CASE("usx_reads_the_text_of_a_verse", "[lua]")
{
  const auto loaded = find_bundle("text-de4e12af7f28f599-245514.zip");
  REQUIRE(loaded != nullptr);
  // The verse sits inside a "wj" character style, which the markup has no node for and takes over as text
  CHECK(
    verse_of(*loaded, book_id::john, 3, 16).value().contains("For God so loved the world, that he gave his only begotten Son")
  );
}

TEST_CASE("usx_keeps_the_headings_of_a_book_out_of_its_verses", "[lua]")
{
  const auto loaded = find_bundle("text-542b32484b6e38c2-246437.zip");
  REQUIRE(loaded != nullptr);
  const auto found = verse_of(*loaded, book_id::chronicles1, 1, 34);
  REQUIRE(found.has_value());
  CHECK(found->contains("Abraham"));
  CHECK(!found->contains("Stammbaum"));
  // The verse sits in a single paragraph, so its markup holds a single section
  const auto section = std::format("<{} ", bible::passage_markup::paragraph);
  CHECK(found->starts_with(section));
  CHECK(found->find(section, section.size()) == std::string::npos);
}

TEST_CASE("usx_tells_where_a_verse_sits_in_its_paragraph", "[lua]")
{
  const auto loaded = find_bundle("text-de4e12af7f28f599-245514.zip");
  REQUIRE(loaded != nullptr);
  CHECK(verse_of(*loaded, book_id::john, 3, 1).value().starts_with(paragraph_open(bible::passage_markup::paragraph_begin)));
  CHECK(verse_of(*loaded, book_id::john, 3, 2).value().starts_with(paragraph_open(bible::passage_markup::paragraph_continue)));
}

TEST_CASE("usx_passages_are_well_formed_markup", "[lua]")
{
  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    const auto& versification = loaded->versification();
    auto checked = std::size_t{0};
    for(auto ref = std::optional{bible::reference::create_unguarded(book_id::john, 1u, 1u)};
        ref && ref->book() == book_id::john;
        ref = versification.next(*ref))
    {
      INFO(std::format("reference: {}", *ref));
      const auto found = loaded->passage(*ref);
      REQUIRE(found.has_value());
      auto document = pugi::xml_document{};
      // The passage is a sequence of paragraph sections, a single root is needed to parse it
      CHECK(document.load_string(std::format("<root>{}</root>", found->content).c_str()));
      ++checked;
    }
    CHECK(checked > 0);
  }
}

TEST_CASE("usx_numbers_the_verses_by_its_text", "[lua]")
{
  // The versification file of this bundle counts a verse 19 its text leaves out
  const auto loaded = find_bundle("text-f492a38d0e52db0f-258505.zip");
  REQUIRE(loaded != nullptr);
  CHECK(loaded->versification().verse_count(book_id::numbers, bible::reference::chapter_type{25}) == 18);
  CHECK(find_bundle("text-de4e12af7f28f599-245514.zip")->versification() == bible::versification_kjv);
}

} // namespace bibstd::lua
