#include <arcstd/util/contains.hpp>
#include <bibstd/bible/reference_parser.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>
#include <vector>

namespace bibstd::bible
{
namespace
{

constexpr auto esv = versification_esv;

///
/// Parse the text with the german language and the ESV versification.
/// \return Parse result
///
auto parse(const std::string_view text, const std::size_t index) -> reference_parser::parse_result
{
  return reference_parser::parse(text, index, arcstd::util::language::german, esv);
}

///
/// \return Reference ranges parsed at the given index
///
auto ranges(const std::string_view text, const std::size_t index) -> std::vector<reference_range>
{
  return parse(text, index).ranges;
}

///
/// \return Reference range covering exactly one verse
///
auto verse(const book_id book, const std::uint32_t chapter, const std::uint32_t number) -> reference_range
{
  return reference_range{reference::create_unguarded(book, chapter, number)};
}

///
/// \return Reference range spanning from the first to the second verse
///
auto verses(
  const book_id book,
  const std::uint32_t chapter_begin,
  const std::uint32_t verse_begin,
  const std::uint32_t chapter_end,
  const std::uint32_t verse_end
) -> reference_range
{
  return reference_range{
    reference::create_unguarded(book, chapter_begin, verse_begin), reference::create_unguarded(book, chapter_end, verse_end)
  };
}

///
/// \return Reference range covering the whole chapter
///
auto chapter(const book_id book, const std::uint32_t number) -> reference_range
{
  return verses(book, number, 1, number, esv.verse_count(book, reference::chapter_type{number}));
}

///
/// \return Substring of the text covered by index_range_origin
///
auto origin(const std::string_view text, const reference_parser::parse_result& result) -> std::string_view
{
  const auto& range = result.index_range_origin;
  return text.substr(range.begin, range.end - range.begin);
}

///
/// Check that the text holds exactly the given reference ranges, whatever their order is.
///
auto check_ranges(const std::string_view text, const std::size_t index, const std::vector<reference_range>& expected) -> void
{
  INFO(std::format("text: \"{}\", index: {}", text, index));
  const auto result = ranges(text, index);
  CHECK(result.size() == expected.size());
  for(const auto& range : expected)
  {
    CHECK(arcstd::util::contains(result, range));
  }
}

} // namespace

TEST_CASE("reference_parser parses a single reference", "[bible]")
{
  SECTION("a chapter and a verse")
  {
    check_ranges("1.Mose 1:1", 0, {verse(book_id::genesis, 1, 1)});
  }
  SECTION("a verse range")
  {
    check_ranges("1.Mose 1;1-2", 0, {verses(book_id::genesis, 1, 1, 1, 2)});
  }
  SECTION("a list of passages")
  {
    check_ranges(
      "2.Mose 2,2-3.7;4,5;5,6-7,8",
      0,
      {verses(book_id::exodus, 2, 2, 2, 3),
       verse(book_id::exodus, 2, 7),
       verse(book_id::exodus, 4, 5),
       verses(book_id::exodus, 5, 6, 7, 8)}
    );
  }
}

TEST_CASE("reference_parser accepts every transition character", "[bible]")
{
  // Every character of `transition_chars` separates a chapter from a verse or a verse from a verse.
  SECTION("between chapter and verse")
  {
    for(const auto separator : std::string_view{",;:"})
    {
      check_ranges(std::format("Joh 3{}16", separator), 0, {verse(book_id::john, 3, 16)});
    }
  }
  SECTION("between verse and verse")
  {
    for(const auto separator : std::string_view{".&+;"})
    {
      check_ranges(std::format("Joh 3,16{}18", separator), 0, {verse(book_id::john, 3, 16), verse(book_id::john, 3, 18)});
    }
  }
}

TEST_CASE("reference_parser expands a reference without a verse", "[bible]")
{
  // A bare chapter covers the whole chapter, a bare chapter range everything in between.
  SECTION("a single chapter")
  {
    check_ranges("1.Mose 5", 0, {chapter(book_id::genesis, 5)});
  }
  SECTION("a chapter range")
  {
    check_ranges(
      "1.Mose 1-2", 0, {verses(book_id::genesis, 1, 1, 2, esv.verse_count(book_id::genesis, reference::chapter_type{2}))}
    );
  }
}

TEST_CASE("reference_parser ignores number postfixes", "[bible]")
{
  for(const auto postfix : std::vector<std::string_view>{"ff.", "f.", "a", "b", "c", "d"})
  {
    check_ranges(std::format("Joh 3,16{}", postfix), 0, {verse(book_id::john, 3, 16)});
  }
}

TEST_CASE("reference_parser rejects references outside the versification", "[bible]")
{
  CHECK(ranges("1.Mose 99,1", 2).empty());
  CHECK(ranges("1.Mose 1,99", 2).empty());
}

TEST_CASE("reference_parser handles degenerate input", "[bible]")
{
  CHECK(ranges("", 0).empty());
  CHECK(ranges("Lorem ipsum dolor sit", 0).empty());

  // An index past the end of the text must not read out of bounds.
  CHECK(ranges("1.Mose 1,1", 10).empty());
  CHECK(ranges("1.Mose 1,1", 1000).empty());

  // A book name without any number is not a reference.
  CHECK(ranges("1.Mose Und", 2).empty());
  CHECK(ranges("steht in 1.Mose", 12).empty());

  // A number too large for std::uint32_t leaves the passage template empty, which is no reference.
  CHECK(ranges("1.Mose 99999999999,1", 2).empty());
}

TEST_CASE("reference_parser resolves the reference under the cursor", "[bible]")
{
  const auto text = std::string_view{"1.Mose 1,1 und 2.Mose 2,2"};
  SECTION("cursor on the first book name")
  {
    const auto result = parse(text, 2);
    CHECK(result.ranges.size() == 1);
    CHECK(arcstd::util::contains(result.ranges, verse(book_id::genesis, 1, 1)));
    CHECK(origin(text, result) == "1.Mose 1,1");
  }
  SECTION("cursor on the numbers of the second reference")
  {
    const auto result = parse(text, 22);
    CHECK(result.ranges.size() == 1);
    CHECK(arcstd::util::contains(result.ranges, verse(book_id::exodus, 2, 2)));
    CHECK(origin(text, result) == "2.Mose 2,2");
  }
  SECTION("cursor on the filler word between both references")
  {
    CHECK(parse(text, 12).ranges.empty());
  }
}

TEST_CASE("reference_parser does not resolve a number behind the reference to it", "[bible]")
{
  // The verse numbers of a bible text follow the reference closing the verse before.
  const auto text = std::string_view{"(5Mo 4,8; Phil 1,10) 19 und getraust dich"};
  SECTION("cursor on the verse number")
  {
    CHECK(parse(text, text.find("19")).ranges.empty());
  }
  SECTION("cursor on the space in front of the verse number")
  {
    CHECK(parse(text, text.find(" 19")).ranges.empty());
  }
  SECTION("cursor on the last number of the reference")
  {
    check_ranges(text, text.find("10"), {verse(book_id::philippians, 1, 10)});
  }
  SECTION("cursor on the parenthesis closing the reference")
  {
    check_ranges(text, text.find(')'), {verse(book_id::philippians, 1, 10)});
  }
}

TEST_CASE("reference_parser does not confuse a book with its numbered variant", "[bible]")
{
  SECTION("without a leading number")
  {
    check_ranges("Joh 1,1", 1, {verse(book_id::john, 1, 1)});
  }
  SECTION("with a leading number")
  {
    check_ranges("1.Joh 1,1", 3, {verse(book_id::john1, 1, 1)});
  }
}

TEST_CASE("reference_parser does not take numbers of a following book", "[bible]")
{
  // The "1" in front of "1.Mose" starts the next reference and must not extend the first one.
  const auto text = std::string_view{"Joh 3,16 1.Mose 1,1"};
  const auto result = parse(text, 1);
  CHECK(result.ranges.size() == 1);
  CHECK(arcstd::util::contains(result.ranges, verse(book_id::john, 3, 16)));
  CHECK(origin(text, result) == "Joh 3,16");
}

TEST_CASE("reference_parser parses a reference split by a line break", "[bible]")
{
  SECTION("between book name and numbers")
  {
    const auto text = std::string_view{"1.Mose\n1,1"};
    const auto result = parse(text, 2);
    CHECK(result.ranges.size() == 1);
    CHECK(arcstd::util::contains(result.ranges, verse(book_id::genesis, 1, 1)));
    CHECK(origin(text, result) == text);
  }
  SECTION("inside the numbers, directly after the transition character")
  {
    check_ranges("Joh 3,\n16", 1, {verse(book_id::john, 3, 16)});
  }
  SECTION("inside a hyphenated book name")
  {
    check_ranges("1.Mo-\nse 1,1 Und", 2, {verse(book_id::genesis, 1, 1)});
    check_ranges("Je-\nsaja 43, 1 und", 1, {verse(book_id::isaiah, 43, 1)});
    check_ranges("Klage-\nlieder 3, 58", 2, {verse(book_id::lamentations, 3, 58)});
  }
  SECTION("inside a reference range")
  {
    check_ranges("Joh 3,16-\n4,2", 1, {verses(book_id::john, 3, 16, 4, 2)});
  }
  SECTION("a word on the next line ends the reference")
  {
    const auto text = std::string_view{"Der Vers steht in Joh 3,16\nund wurde oft zitiert"};
    const auto result = parse(text, 20);
    CHECK(result.ranges.size() == 1);
    CHECK(arcstd::util::contains(result.ranges, verse(book_id::john, 3, 16)));
    CHECK(origin(text, result) == "Joh 3,16");
  }
}

TEST_CASE("reference_parser reads a reference spaced out around its separators", "[bible]")
{
  SECTION("around a separator between verses")
  {
    // Which character separates the numbers is only decided later by match_passage_template, so a gap
    // has to be crossed for every transition character, no matter which one it turns out to be. The
    // full stop is the exception, \see the sentence punctuation test below.
    for(const auto& text :
        std::vector<std::string_view>{"Joh 3, 16.18", "Joh 3,16 .18", "Joh 3,16 ;18", "Joh 3,16; 18", "Joh 3,16\n.18"})
    {
      check_ranges(text, 1, {verse(book_id::john, 3, 16), verse(book_id::john, 3, 18)});
    }
  }
  SECTION("around a verse range")
  {
    for(const auto& text : std::vector<std::string_view>{"Joh 3,16-18", "Joh 3,16 -18", "Joh 3,16 - 18"})
    {
      check_ranges(text, 1, {verses(book_id::john, 3, 16, 3, 18)});
    }
  }
  SECTION("around a chapter range")
  {
    // A spaced out chapter range must not collapse to its first chapter.
    check_ranges(
      "1.Mose 1 - 2", 2, {verses(book_id::genesis, 1, 1, 2, esv.verse_count(book_id::genesis, reference::chapter_type{2}))}
    );
  }
  SECTION("in front of an unrelated number on the next line")
  {
    // A spaced out verse list followed by an unrelated number keeps the whole list.
    check_ranges("Joh 3, 16.18\n12 Personen folgten", 1, {verse(book_id::john, 3, 16), verse(book_id::john, 3, 18)});
  }
}

TEST_CASE("reference_parser does not absorb an unrelated number from the next line", "[bible]")
{
  const auto text = std::string_view{"Wie in Johannes 3,16\n12 Junger folgten ihm"};
  const auto result = parse(text, 9);
  CHECK(result.ranges.size() == 1);
  CHECK(arcstd::util::contains(result.ranges, verse(book_id::john, 3, 16)));
  CHECK(origin(text, result) == "Johannes 3,16");
}

TEST_CASE("reference_parser does not shift a verse across a line break", "[bible]")
{
  // The absorbed number stays inside the versification here, so the reference is not rejected but
  // silently answered with 1.Mose 1,12.
  check_ranges("1.Mose 1,1\n2 Und Gott sprach", 2, {verse(book_id::genesis, 1, 1)});
}

TEST_CASE("reference_parser does not glue numbers separated by a word", "[bible]")
{
  // The word consists only of postfix letters, so it does not end the numbers range either.
  check_ranges("1.Mose 1,1 ab 2,3", 2, {verse(book_id::genesis, 1, 1)});
}

TEST_CASE("reference_parser keeps a reference with a postfix intact", "[bible]")
{
  // The stripped "ff." must not let the next line's number attach to the verse.
  check_ranges("Joh 3,16ff.\n5 Personen waren dabei", 1, {verse(book_id::john, 3, 16)});
}

TEST_CASE("reference_parser does not read a sentence-final number as a verse", "[bible]")
{
  SECTION("a full stop followed by a gap ends a sentence")
  {
    check_ranges("siehe Joh 3,16.\n5 Personen waren dabei", 7, {verse(book_id::john, 3, 16)});
    check_ranges("1.Mose 1,1. 5 Leute waren dabei", 2, {verse(book_id::genesis, 1, 1)});
  }
  SECTION("a full stop without a gap separates passage numbers")
  {
    check_ranges("Joh 3,16.18", 1, {verse(book_id::john, 3, 16), verse(book_id::john, 3, 18)});
  }
  SECTION("a verse list written with a gap after the full stop loses its tail")
  {
    // Accepted trade-off: it cannot be told apart from a sentence boundary.
    check_ranges("Joh 3,16. 18", 1, {verse(book_id::john, 3, 16)});
  }
}

} // namespace bibstd::bible
