#include "test_utils/script_sandbox.hpp"
#include "test_utils/scripture_store.hpp"
#include "test_utils/zip_archive.hpp"

#include <bibstd/bible/common.hpp>
#include <bibstd/bible/reference.hpp>
#include <bibstd/bible/scripture.hpp>
#include <bibstd/bible/versification.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::lua
{
namespace
{

using bible::book_id;
using test_utils::read_scripture;
using test_utils::verse_of;

///
/// \return the scripture of a USFM archive, Genesis replaced by \p genesis
///
[[nodiscard]] auto read_genesis(const std::string_view name, const std::string& genesis)
  -> std::shared_ptr<const bible::scripture>
{
  return read_scripture(
    name,
    test_utils::usfm_entries({
      {"GEN", genesis}
  })
  );
}

// One document in USFM and in USX
constexpr auto usfm = R"(\id GEN Test
\usfm 3.1
\h Genesis
\c 1 \ca 2\ca*
\cp A
\s1 Heading // here
\p
\v 1 In the \nd Lord\nd*\f + \fr 1.1: \ft Note\f* and \w word|lemma="w"\w* \ref Mk 9.50|MRK 9:50\ref*.
\v 2 \va 3\va* a~b \fig Caption|src="a.png" size="col"\fig*
\q1 \qt-s |who="God"\*come\qt-e\*
\tr \th1 Head \tcr2 Cell
\tr \tc1 One
\esb \cat History\cat*
\p Side
\esbe
)";
constexpr auto usx =
  R"(<usx version="3.1"><book code="GEN" style="id">Test</book><para style="h">Genesis</para>)"
  R"(<chapter number="1" style="c" altnumber="2" pubnumber="A"/><para style="s1">Heading <optbreak/> here</para>)"
  R"(<para style="p"><verse number="1" style="v"/>In the <char style="nd">Lord</char><note caller="+" style="f">)"
  R"(<char style="fr">1.1: </char><char style="ft">Note</char></note> and <char style="w" lemma="w">word</char> )"
  R"(<ref loc="MRK 9:50">Mk 9.50</ref>. <verse number="2" style="v" altnumber="3"/> a)"
  "\xC2\xA0"
  R"(b <figure style="fig" file="a.png" size="col">Caption</figure></para>)"
  R"(<para style="q1"><ms style="qt-s" who="God"/>come<ms style="qt-e"/></para>)"
  R"(<table><row style="tr"><cell style="th1" align="start">Head</cell><cell style="tcr2" align="end">Cell</cell>)"
  R"(</row><row style="tr"><cell style="tc1" align="start">One</cell></row></table>)"
  R"(<sidebar style="esb" category="History"><para style="p">Side</para></sidebar></usx>)";

} // namespace

TEST_CASE("usfm_reads_the_verses_of_paragraphs", "[lua]")
{
  const auto scripture = read_genesis(
    "usfm_reads_the_verses_of_paragraphs",
    R"(\id GEN Genesis
\usfm 3.1
\h Genesis
\toc1 The First Book of Moses
\toc2 Genesis
\toc3 Gen
\mt1 Genesis
\is Introduction
\ip An introduction.
\c 1
\s1 The Creation
\r (John 1.1-3)
\p
\v 1 In the beginning
\v 2 And the earth
\q1 was without form,
\q2 and void.
\p
\v 3 And God said
)"
  );
  REQUIRE(scripture);
  // Headings and introductions are no text of a verse
  CHECK(verse_of(*scripture, book_id::genesis, 1, 1) == R"(<p data-id="begin">In the beginning </p>)");
  // A verse in several paragraphs has a section in each
  CHECK(
    verse_of(*scripture, book_id::genesis, 1, 2) ==
    R"(<p data-id="continue">And the earth</p><p data-id="begin">was without form,</p><p data-id="begin">and void.</p>)"
  );
  CHECK(verse_of(*scripture, book_id::genesis, 1, 3) == R"(<p data-id="begin">And God said</p>)");

  const auto names = scripture->book_information(book_id::genesis);
  REQUIRE(names);
  CHECK(names->abbreviation == "Gen");
  CHECK(names->short_name == "Genesis");
  CHECK(names->long_name == "The First Book of Moses");
}

TEST_CASE("usfm_keeps_the_notes_out_of_the_text", "[lua]")
{
  const auto scripture = read_genesis(
    "usfm_keeps_the_notes_out_of_the_text",
    R"(\id GEN
\c 1
\p
\v 1 Salt\f + \fr 1.1: \fk Salt: \ft A note with \nd Lord\nd*.\f* and light\x - \xo 1.1: \xt Mk 9.50; Lk 14.34\x*.
)"
  );
  REQUIRE(scripture);
  const auto passage = scripture->passage(bible::reference::create_unguarded(book_id::genesis, 1u, 1u));
  REQUIRE(passage);
  CHECK(passage->content == R"(<p data-id="begin">Salt and light.</p>)");
  // The target references of a cross reference are kept
  CHECK(passage->cross_references == std::vector<std::string>{"Mk 9.50; Lk 14.34"});
}

TEST_CASE("usfm_marks_character_styles_up", "[lua]")
{
  const auto scripture = read_genesis(
    "usfm_marks_character_styles_up",
    R"(\id GEN
\c 1
\p
\v 1 the \wj word of the \nd Lord\nd*\wj* is \w gracious|lemma="grace"\w*, \+add said\+add* he \qt-s |who="God"\*come\qt-e\*.
\v 2 a & b < c
)"
  );
  REQUIRE(scripture);
  // A style without markup is text, attributes and milestones are no text
  CHECK(
    verse_of(*scripture, book_id::genesis, 1, 1) ==
    R"(<p data-id="begin">the word of the <i>Lord</i> is gracious, said he come. </p>)"
  );
  CHECK(verse_of(*scripture, book_id::genesis, 1, 2) == R"(<p data-id="continue">a &amp; b &lt; c</p>)");
}

TEST_CASE("usfm_resolves_escapes_and_spaces", "[lua]")
{
  const auto scripture = read_genesis(
    "usfm_resolves_escapes_and_spaces",
    R"(\id GEN
\c 1
\p
\v 1 a~b \\ c\~//d
\v 2 \w a\|b|lemma="c"\w*
)"
  );
  REQUIRE(scripture);
  // "~" is a no-break space, "//" an optional line break
  CHECK(
    verse_of(*scripture, book_id::genesis, 1, 1) == "<p data-id=\"begin\">a\xC2\xA0"
                                                    "b \\ c~d </p>"
  );
  // An escaped bar is text, the first bar that is none starts the attributes
  CHECK(verse_of(*scripture, book_id::genesis, 1, 2) == R"(<p data-id="continue">a|b</p>)");
}

TEST_CASE("usfm_leaves_other_numberings_out", "[lua]")
{
  const auto scripture = read_genesis(
    "usfm_leaves_other_numberings_out",
    R"(\id GEN
\c 1 \ca 2\ca*
\cp A
\p
\v 1 \va 3\va*\vp 1b\vp* text
)"
  );
  REQUIRE(scripture);
  CHECK(verse_of(*scripture, book_id::genesis, 1, 1) == R"(<p data-id="begin"> text</p>)");
}

TEST_CASE("usfm_derives_the_versification_from_the_verses", "[lua]")
{
  const auto scripture = read_genesis(
    "usfm_derives_the_versification_from_the_verses",
    R"(\id GEN
\c 1
\p
\v 1 one
\v 2-4 two to four
\c 2
\p
\v 1 a
\v 2
)"
  );
  REQUIRE(scripture);
  const auto& versification = scripture->versification();
  CHECK(versification.chapter_count(book_id::genesis) == 2);
  // A bridged verse numbers its verses, its text is the one of its first verse
  CHECK(versification.verse_count(book_id::genesis, bible::reference::chapter_type{1}) == 4);
  CHECK(verse_of(*scripture, book_id::genesis, 1, 2) == R"(<p data-id="continue">two to four</p>)");
  CHECK(verse_of(*scripture, book_id::genesis, 1, 3) == "");
  CHECK(verse_of(*scripture, book_id::genesis, 1, 4) == "");
  // So does a verse without text
  CHECK(versification.verse_count(book_id::genesis, bible::reference::chapter_type{2}) == 2);
  CHECK(verse_of(*scripture, book_id::genesis, 2, 2) == "");
}

TEST_CASE("usfm_ignores_a_versification_file", "[lua]")
{
  // The text numbers the verses, not the file
  auto entries = test_utils::usfm_entries();
  entries.emplace_back("versification.vrs", "GEN 1:31 2:25\n");
  const auto scripture = read_scripture("usfm_ignores_a_versification_file", entries);
  REQUIRE(scripture);
  CHECK(scripture->versification().chapter_count(book_id::genesis) == 1);
  CHECK(scripture->versification().verse_count(book_id::genesis, bible::reference::chapter_type{1}) == 1);
}

TEST_CASE("usfm_reads_the_paratext_settings", "[lua]")
{
  auto entries = test_utils::usfm_entries();
  entries.emplace_back(
    "Settings.xml",
    "<ScriptureText><Name>TST</Name><FullName>Test Bible</FullName><Language>German</Language>"
    "<Copyright>Public Domain</Copyright></ScriptureText>"
  );
  const auto scripture = read_scripture("usfm_reads_the_paratext_settings", entries);
  REQUIRE(scripture);
  const auto& info = scripture->information();
  CHECK(info.name == "Test Bible");
  CHECK(info.abbreviation == "TST");
  CHECK(info.language == "German");
  CHECK(info.copyright == "Public Domain");
}

TEST_CASE("usfm_reads_a_bundle_of_the_digital_bible_library", "[lua]")
{
  // A bundle with USFM files instead of USX, its metadata names the scripture
  auto entries = test_utils::usfm_entries();
  entries.emplace_back(
    "metadata.xml", "<DBLMetadata><identification><nameLocal>Bundle</nameLocal></identification></DBLMetadata>"
  );
  const auto scripture = read_scripture("usfm_reads_a_bundle_of_the_digital_bible_library", entries);
  REQUIRE(scripture);
  CHECK(scripture->information().name == "Bundle");
}

TEST_CASE("usfm_reads_an_archive_of_ebible", "[lua]")
{
  // An archive of eBible.org, its copyright page names the scripture
  auto entries = test_utils::usfm_entries();
  entries.emplace_back(
    "copr.htm",
    R"(<div class="toc"><a href="https://ebible.org/tst">Test</a></div><h1>Test Bible</h1><p>Public Domain<br /></p>)"
  );
  const auto scripture = read_scripture("usfm_reads_an_archive_of_ebible", entries);
  REQUIRE(scripture);
  const auto& info = scripture->information();
  CHECK(info.name == "Test Bible");
  CHECK(info.abbreviation == "tst");
  CHECK(info.copyright == "Public Domain");
}

TEST_CASE("usfm_reads_a_new_testament", "[lua]")
{
  // From Matthew on, the books left out take the closest default versification and have no passages
  const auto entries = test_utils::usfm_entries() | std::views::drop(39) | std::ranges::to<std::vector>();
  const auto scripture = read_scripture("usfm_reads_a_new_testament", entries);
  REQUIRE(scripture);
  CHECK(verse_of(*scripture, book_id::matthew, 1, 1) == R"(<p data-id="begin">MAT 1:1</p>)");
  CHECK_FALSE(verse_of(*scripture, book_id::genesis, 1, 1));
  CHECK(scripture->versification().chapter_count(book_id::matthew) == 1);
  CHECK(scripture->versification().chapter_count(book_id::genesis) == 50);
  CHECK(scripture->versification().verse_count(book_id::genesis, bible::reference::chapter_type{1}) == 31);
}

TEST_CASE("usfm_fills_a_scripture_with_the_closest_default_versification", "[lua]")
{
  // 3 John ends with verse 15 in the ESV and 14 in the KJV, the other books are taken from the one it matches
  const auto third_john = [](const std::uint32_t verses)
  {
    auto text = std::string{"\\id 3JN\n\\c 1\n\\p\n"};
    for(auto verse = 1U; verse <= verses; ++verse)
    {
      text += std::format("\\v {} Text\n", verse);
    }
    return std::vector<io::test_utils::zip_entry>{{"3JN.usfm", text}};
  };
  const auto esv = read_scripture("usfm_fills_a_scripture_with_the_esv", third_john(15));
  const auto kjv = read_scripture("usfm_fills_a_scripture_with_the_kjv", third_john(14));
  REQUIRE(esv);
  REQUIRE(kjv);
  CHECK(esv->versification() == bible::versification_esv);
  CHECK(kjv->versification() == bible::versification_kjv);
}

TEST_CASE("usfm_rejects_an_archive_without_a_book_of_the_app", "[lua]")
{
  const auto entries = std::vector<io::test_utils::zip_entry>{
    {"XXA.usfm", "\\id XXA\n\\c 1\n\\p\n\\v 1 Extra\n"}
  };
  CHECK_FALSE(read_scripture("usfm_rejects_an_archive_without_a_book_of_the_app", entries));
}

TEST_CASE("usfm_gives_the_tree_and_book_of_usx", "[lua]")
{
  auto fixture = test_utils::script_sandbox{"usfm_gives_the_tree_and_book_of_usx"};
  fixture.texts = {
    {"usfm_text", usfm},
    { "usx_text",  usx}
  };
  const auto dump = [&](const std::string& code) { return fixture.value("util.dump(" + code + ")").as<std::string>(); };
  CHECK(dump("scripts.usfm.parse(usfm_text)") == dump("scripts.usx.parse(usx_text)"));
  CHECK(dump("scripts.usfm.book(scripts.usfm.parse(usfm_text))") == dump("scripts.usx.book(scripts.usx.parse(usx_text))"));

  CHECK(fixture.check("select(2, scripts.usfm.parse('\\\\p no book')) == 'no book identification'"));
  CHECK(fixture.check("select(2, scripts.usx.parse('<other/>')) == 'no usx root element'"));
  CHECK(fixture.check("scripts.usx.books().GEN == 'genesis' and scripts.usfm.books().REV == 'revelation'"));

  const auto book = [&](const std::string& code)
  {
    return fixture.check("(function() local book = scripts.usx.book(scripts.usx.parse(usx_text)) return " + code + " end)()");
  };
  CHECK(book("book.id == 'genesis' and book.code == 'GEN' and book.names.short_name == 'Genesis'"));
  CHECK(book(R"(book.verses[1][1] == '<p data-id="begin">In the Lord and word Mk 9.50. </p>')"));
  CHECK(book("#book.verses == 1 and #book.verses[1] == 2"));
}

// Known defects of util.usx_book, taken over from the former C++ reader

TEST_CASE("usfm_keeps_the_markup_of_a_character_style_in_a_paragraph", "[lua][!shouldfail]")
{
  const auto scripture = read_genesis(
    "usfm_keeps_the_markup_of_a_character_style_in_a_paragraph",
    R"(\id GEN
\c 1
\p
\v 1 the \nd Lord\nd*
)"
  );
  REQUIRE(scripture);
  CHECK(verse_of(*scripture, book_id::genesis, 1, 1) == R"(<p data-id="begin">the <i>Lord</i></p>)");
}

TEST_CASE("usfm_keeps_figures_out_of_the_text", "[lua][!shouldfail]")
{
  const auto scripture = read_genesis(
    "usfm_keeps_figures_out_of_the_text",
    R"(\id GEN
\c 1
\p
\v 1 text \fig Caption|src="a.png" size="col"\fig*
)"
  );
  REQUIRE(scripture);
  CHECK(verse_of(*scripture, book_id::genesis, 1, 1) == R"(<p data-id="begin">text </p>)");
}

TEST_CASE("usfm_keeps_sidebars_out_of_the_text", "[lua][!shouldfail]")
{
  const auto scripture = read_genesis(
    "usfm_keeps_sidebars_out_of_the_text",
    R"(\id GEN
\c 1
\p
\v 1 text
\esb
\p Side
\esbe
)"
  );
  REQUIRE(scripture);
  CHECK(verse_of(*scripture, book_id::genesis, 1, 1) == R"(<p data-id="begin">text</p>)");
}

} // namespace bibstd::lua
