#include "test_utils/clipboard.hpp"
#include "test_utils/files.hpp"
#include "test_utils/scripts.hpp"
#include "test_utils/scripture_names.hpp"
#include "test_utils/temp_folder.hpp"
#include "test_utils/wait_until.hpp"

#include <arcstd/lua/lua_engine.hpp>
#include <bibstd/lua/bundled_scripts.hpp>
#include <bibstd/system/locale.hpp>
#include <bibstd/workflow/workflow_bible_ref_copy.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>

namespace bibstd::workflow
{
namespace
{

// Offers the scripture ABC, without chapter 5, with all names of john and the long name of luke only
constexpr auto scripture_script = R"(
  return {
    name = "Scripture",
    description = "",
    node = "scripture",
    ["scripture.names"] = function() return {names = {"ABC"}} end,
    ["scripture.information"] = function() return {abbreviation = "A", copyright = "(c) ABC"} end,
    ["scripture.book"] = function(input)
      if input.book == "john" then
        return {abbreviation = "Joh", short_name = "Johannes", long_name = "Evangelium nach Johannes"}
      end
      if input.book == "luke" then
        return {long_name = "Evangelium nach Lukas"}
      end
    end,
    ["scripture.passage"] = function(input)
      if input.chapter == 5 then return {error = "no such chapter"} end
      return {text = "<Vers " .. input.verse .. ">"}
    end,
  }
)";

///
/// Script, scripture and copy workflow, with scripture_script if \p with_scripture.
///
struct copy_fixture final
{
  bibstd::test_utils::clipboard_restorer clipboard;
  arcstd::test_utils::temp_folder folder;
  std::shared_ptr<arcstd::lua::lua_engine> script;
  std::shared_ptr<workflow_scripture> scripture;
  std::shared_ptr<workflow_bible_ref_copy> copy;

  explicit copy_fixture(std::string_view name, bool with_scripture);
};

///
///
copy_fixture::copy_fixture(const std::string_view name, const bool with_scripture)
  : folder{name}
{
  const auto scripts_folder = folder.path() / "scripts";
  std::filesystem::create_directories(scripts_folder);
  if(with_scripture)
  {
    arcstd::test_utils::write_file(scripts_folder / "scripture.lua", scripture_script);
  }
  const auto settings = arcstd::test_utils::make_script_settings(folder.path(), with_scripture);
  script = std::make_shared<arcstd::lua::lua_engine>(settings, lua::bundled::all);
  scripture = std::make_shared<workflow_scripture>(settings, script);
  copy = std::make_shared<workflow_bible_ref_copy>(settings, scripture);
  // Last, as the app does
  arcstd::test_utils::load_scripts(*script);
}

///
/// \return the verses of john from \p chapter_begin, \p verse_begin to \p chapter_end, \p verse_end
///
[[nodiscard]] auto range_of(
  const std::uint32_t chapter_begin,
  const std::uint32_t verse_begin,
  const std::uint32_t chapter_end,
  const std::uint32_t verse_end
) -> workflow_bible_ref_copy::params
{
  const auto range = bible::reference_range{
    bible::reference::create_unguarded(bible::book_id::john, chapter_begin, verse_begin),
    bible::reference::create_unguarded(bible::book_id::john, chapter_end, verse_end),
  };
  return {{.range = range}};
}

///
/// \return the verses from the start of \p book_begin to the start of \p book_end
///
[[nodiscard]] auto books_of(const bible::book_id book_begin, const bible::book_id book_end) -> workflow_bible_ref_copy::params
{
  const auto range = bible::reference_range{
    bible::reference::create_unguarded(book_begin, 1, 1),
    bible::reference::create_unguarded(book_end, 1, 1),
  };
  return {{.range = range}};
}

///
/// \return what copying \p params leaves in the clipboard, std::nullopt if nothing was copied
///
[[nodiscard]] auto copied(const copy_fixture& fixture, const workflow_bible_ref_copy::params& params)
  -> std::optional<std::string>
{
  return fixture.copy->copy(params) ? bibstd::test_utils::clipboard_text() : std::nullopt;
}

} // namespace

// Hidden, they replace the clipboard of whoever runs them. Run them with: bibstd_test "[.clipboard]"
TEST_CASE("workflow_bible_ref_copy_defaults_to_the_reference", "[.clipboard][workflow]")
{
  const auto fixture = copy_fixture{"workflow_bible_ref_copy_defaults_to_the_reference", true};
  REQUIRE(
    arcstd::test_utils::wait_until([&]()
                                   { return test_utils::scripture_names_of(*fixture.scripture, "Scripture").size() == 1; })
  );
  REQUIRE(fixture.scripture->settings().default_scripture_name->value("ABC (Scripture)"));
  const auto german = bibstd::system::locale::preferred_language() == bibocr::util::language::german;
  // The verses, then the reference and the abbreviation of the scripture
  const auto separator = german ? "," : ":";
  CHECK(
    copied(fixture, range_of(3, 16, 3, 18)) ==
    std::format("\"<Vers 16> <Vers 17> <Vers 18>\" (Johannes\u00A03{} 16\u00A0- 18 A)", separator)
  );
  CHECK(copied(fixture, range_of(3, 16, 3, 16)) == std::format("\"<Vers 16>\" (Johannes\u00A03{} 16 A)", separator));
  CHECK(copied(fixture, range_of(3, 16, 4, 2))
          .value_or("")
          .ends_with(std::format("(Johannes\u00A03{0} 16\u00A0- 4{0}2 A)", separator)));
  // The scripture does not name this book, so it is left out
  CHECK(
    copied(fixture, books_of(bible::book_id::mark, bible::book_id::mark)) == std::format("\"<Vers 1>\" (1{} 1 A)", separator)
  );
}

TEST_CASE("workflow_bible_ref_copy_writes_through_the_mask", "[.clipboard][workflow]")
{
  const auto fixture = copy_fixture{"workflow_bible_ref_copy_writes_through_the_mask", true};
  const auto& settings = fixture.copy->settings();
  REQUIRE(
    arcstd::test_utils::wait_until([&]()
                                   { return test_utils::scripture_names_of(*fixture.scripture, "Scripture").size() == 1; })
  );
  REQUIRE(fixture.scripture->settings().default_scripture_name->value("ABC (Scripture)"));

  SECTION("with the names of the scripture")
  {
    REQUIRE(settings.mask->value("[book]|[book_long]|[book_abbr]|[scripture]|[scripture_abbr]|[copyright]"));
    const auto expected = "Johannes|Evangelium nach Johannes|Joh|ABC (Scripture)|A|(c) ABC";
    CHECK(copied(fixture, range_of(3, 16, 3, 16)) == expected);
  }

  SECTION("without the names the scripture does not have")
  {
    REQUIRE(settings.mask->value("x{ [book]}{ ([book_abbr])}{ [book_long]}"));
    CHECK(copied(fixture, books_of(bible::book_id::luke, bible::book_id::luke)) == "x Evangelium nach Lukas");
    CHECK(copied(fixture, books_of(bible::book_id::mark, bible::book_id::mark)) == "x");
  }

  SECTION("with the text of the verses")
  {
    REQUIRE(settings.mask->value("\"[content]\"[newline][content_numbered]"));
    const auto expected = "\"<Vers 35> <Vers 36> <Vers 1>\"\n35 <Vers 35> 36 <Vers 36> 1 <Vers 1>";
    CHECK(copied(fixture, range_of(3, 35, 4, 1)) == expected);
  }

  SECTION("not without a verse it needs")
  {
    REQUIRE(settings.mask->value("[content]"));
    CHECK_FALSE(copied(fixture, range_of(4, 54, 5, 1)).has_value());
  }

  SECTION("without asking for verses it does not need")
  {
    REQUIRE(settings.mask->value("[book] [#chapter_begin]"));
    CHECK(copied(fixture, range_of(5, 1, 5, 2)) == "Johannes 5");
  }

  SECTION("not if nothing is left")
  {
    REQUIRE(settings.mask->value(""));
    CHECK_FALSE(copied(fixture, range_of(3, 16, 3, 16)).has_value());
  }
}

} // namespace bibstd::workflow
