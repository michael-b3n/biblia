#include <bibstd/bible/reference_mask.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace bibstd::bible
{
namespace
{

// The default of the setting "copy.mask" in german
constexpr auto reference_only = "[book] [#chapter_begin],[#verse_begin]{-{[book_end] }{[#chapter_end],}[#verse_end]}";

constexpr auto ref(const book_id book, const std::uint32_t chapter, const std::uint32_t verse) -> reference
{
  return reference::create_unguarded(book, chapter, verse);
}

auto values_of(const reference_range& range) -> reference_mask::values
{
  return {
    .range = range,
    .content = "Text",
    .content_numbered = "16 Text",
    .book = "Johannes",
    .book_long = "Das Evangelium nach Johannes",
    .book_abbreviation = "Joh",
    .book_end = "Apostelgeschichte",
    .scripture = "Lutherbibel",
    .scripture_abbreviation = "LUT",
    .copyright = "",
  };
}

auto render(const std::string_view mask, const reference_range& range) -> std::string
{
  return reference_mask::render(mask, values_of(range));
}

constexpr auto verse = reference_range{ref(book_id::john, 3, 16)};
constexpr auto verses = reference_range{ref(book_id::john, 3, 16), ref(book_id::john, 3, 18)};
constexpr auto chapters = reference_range{ref(book_id::john, 3, 16), ref(book_id::john, 4, 2)};
constexpr auto books = reference_range{ref(book_id::john, 21, 25), ref(book_id::acts, 1, 1)};

} // namespace

TEST_CASE("reference_mask replaces its variables", "[bible]")
{
  CHECK(render("[book]", verse) == "Johannes");
  CHECK(render("[book_long]", verse) == "Das Evangelium nach Johannes");
  CHECK(render("[book_abbr]", verse) == "Joh");
  CHECK(render("[book_end]", books) == "Apostelgeschichte");
  CHECK(render("[content]", verse) == "Text");
  CHECK(render("[content_numbered]", verse) == "16 Text");
  CHECK(render("[#chapter_begin]", verse) == "3");
  CHECK(render("[#verse_begin]", verse) == "16");
  CHECK(render("[#chapter_end]", chapters) == "4");
  CHECK(render("[#verse_end]", chapters) == "2");
  CHECK(render("[scripture]", verse) == "Lutherbibel");
  CHECK(render("[scripture_abbr]", verse) == "LUT");
}

TEST_CASE("reference_mask inserts special characters", "[bible]")
{
  CHECK(render("[book][nbsp]3", verse) == "Johannes\u00A03");
  CHECK(render("a[newline]b", verse) == "a\nb");
  CHECK(render("a[tab]b", verse) == "a\tb");
}

TEST_CASE("reference_mask keeps what it does not know as typed", "[bible]")
{
  CHECK(render("\u201E[content]\u201C\u00A0(1)", verse) == "\u201EText\u201C\u00A0(1)");
  CHECK(render("[1] [Buch] [] [", verse) == "[1] [Buch] [] [");
  CHECK(render("a} {b", verse) == "a} {b");
  CHECK(render("{[book]", verse) == "{Johannes");
  CHECK(render("[book]}", verse) == "Johannes}");
  CHECK(render("", verse).empty());
}

TEST_CASE("reference_mask only reads a mask once", "[bible]")
{
  auto values = values_of(verse);
  values.content = "[book] {x}";
  CHECK(reference_mask::render("[content]", values) == "[book] {x}");
}

TEST_CASE("reference_mask leaves out end values repeating the begin", "[bible]")
{
  CHECK(render(reference_only, verse) == "Johannes 3,16");
  CHECK(render(reference_only, verses) == "Johannes 3,16-18");
  CHECK(render(reference_only, chapters) == "Johannes 3,16-4,2");
  CHECK(render(reference_only, books) == "Johannes 21,25-Apostelgeschichte 1,1");
}

TEST_CASE("reference_mask keeps the end verse equal to the begin verse of another chapter", "[bible]")
{
  const auto range = reference_range{ref(book_id::john, 3, 16), ref(book_id::john, 4, 16)};
  CHECK(render(reference_only, range) == "Johannes 3,16-4,16");
}

TEST_CASE("reference_mask keeps the end chapter equal to the begin chapter of another book", "[bible]")
{
  const auto range = reference_range{ref(book_id::john, 1, 2), ref(book_id::acts, 1, 3)};
  CHECK(render(reference_only, range) == "Johannes 1,2-Apostelgeschichte 1,3");
}

TEST_CASE("reference_mask copies a whole chapter as a verse range", "[bible]")
{
  const auto chapter = reference_range{ref(book_id::john, 3, 1), ref(book_id::john, 3, 36)};
  CHECK(render(reference_only, chapter) == "Johannes 3,1-36");
}

TEST_CASE("reference_mask drops a group only if all its variables are empty", "[bible]")
{
  CHECK(render("a{-[#verse_end]}b", verse) == "ab");
  CHECK(render("a{-[#verse_end]}b", verses) == "a-18b");
  CHECK(render("a{-[#chapter_end][#verse_end]}b", verses) == "a-18b");
  CHECK(render("a{ ([copyright])}b", verse) == "ab");
}

TEST_CASE("reference_mask keeps a group without variables as typed", "[bible]")
{
  CHECK(render("{x}", verse) == "{x}");
  CHECK(render("{}", verse) == "{}");
  CHECK(render("{[nbsp]}", verse) == "{\u00A0}");
  CHECK(render("{[1]}", verse) == "{[1]}");
  CHECK(render("{[book]{x}}", verse) == "Johannes{x}");
}

TEST_CASE("reference_mask drops a group whose nested groups all drop", "[bible]")
{
  CHECK(render("a{-{[book_end] }{[#chapter_end],}}b", verses) == "ab");
  CHECK(render("a{-{[book_end] }{[#chapter_end],}}b", chapters) == "a-4,b");
}

TEST_CASE("reference_mask tells whether a mask needs the verses", "[bible]")
{
  CHECK(reference_mask::uses_content("[content]"));
  CHECK(reference_mask::uses_content("\"[content]\""));
  CHECK(reference_mask::uses_content("[content_numbered]"));
  CHECK(reference_mask::uses_content("{[content_numbered]}"));
  CHECK_FALSE(reference_mask::uses_content(reference_only));
  CHECK_FALSE(reference_mask::uses_content("[Inhalt]"));
}

} // namespace bibstd::bible
