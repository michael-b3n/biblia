#include <bibstd/bible/scripture_types.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

namespace bibstd::bible
{
namespace
{

auto section(const std::string_view content) -> std::string
{
  return passage_markup::section(passage_markup::paragraph_begin, content);
}

} // namespace

TEST_CASE("passage_markup::to_plain drops the markup", "[bible]")
{
  CHECK(
    passage_markup::to_plain(section("Denn also hat <i>Gott</i> die <b>Welt</b> geliebt")) ==
    "Denn also hat Gott die Welt geliebt"
  );
  CHECK(passage_markup::to_plain("") == "");
}

TEST_CASE("passage_markup::to_plain reads the escaped characters", "[bible]")
{
  const auto text = std::string{"a < b & c > d &amp; e"};
  CHECK(passage_markup::to_plain(section(passage_markup::escaped_xml(text))) == text);
}

TEST_CASE("passage_markup::to_plain separates the sections", "[bible]")
{
  const auto content = section("erster Absatz") + section("zweiter Absatz");
  CHECK(passage_markup::to_plain(content) == "erster Absatz zweiter Absatz");
}

TEST_CASE("passage_markup::to_plain collapses whitespace", "[bible]")
{
  CHECK(passage_markup::to_plain(section("  a \n  b\t\tc  ")) == "a b c");
  CHECK(passage_markup::to_plain(section("a ") + section(" b")) == "a b");
}

TEST_CASE("passage_markup::to_plain keeps what is no xml as it is", "[bible]")
{
  CHECK(passage_markup::to_plain("<p>open") == "<p>open");
}

TEST_CASE("passage_markup::to_plain keeps characters beyond ASCII", "[bible]")
{
  CHECK(passage_markup::to_plain(section("für\u00A0uns")) == "für\u00A0uns");
  CHECK(passage_markup::to_plain(section("a\u200Db")) == "a\u200Db");
}

TEST_CASE("passage_markup::to_xml escapes the text", "[bible]")
{
  CHECK(passage_markup::to_xml("a < b") == R"(<p data-id="undefined">a &lt; b</p>)");
  CHECK(passage_markup::to_plain(passage_markup::to_xml("a < b")) == "a < b");
}

TEST_CASE("passage offers xml content in any format", "[bible]")
{
  const auto verse = passage{reference::create_unguarded(book_id::john, 3, 16), section("a <i>b</i>"), passage_format::xml};
  CHECK(verse.xml() == section("a <i>b</i>"));
  CHECK(verse.plain() == "a b");
}

TEST_CASE("passage offers plain content in any format", "[bible]")
{
  const auto verse = passage{reference::create_unguarded(book_id::john, 3, 16), "a < b", passage_format::plain};
  CHECK(verse.plain() == "a < b");
  CHECK(verse.xml() == R"(<p data-id="undefined">a &lt; b</p>)");
}

TEST_CASE("passage keeps content that is no xml as it is", "[bible]")
{
  CHECK(passage{reference::create_unguarded(book_id::john, 3, 16), "<p>open", passage_format::xml}.plain() == "<p>open");
}

} // namespace bibstd::bible
