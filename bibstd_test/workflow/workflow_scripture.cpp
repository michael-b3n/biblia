#include "test_utils/files.hpp"
#include "test_utils/script_settings.hpp"
#include "test_utils/temp_folder.hpp"

#include <bibstd/workflow/workflow_script.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <format>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace bibstd::workflow
{
namespace
{

// Offers the scriptures AAA and BBB
constexpr auto scripture_script = R"(
  return {
    ["scripture.names"] = function() return {names = {"AAA", "BBB"}} end,
    ["scripture.information"] = function(input)
      return {abbreviation = input.name, language = "de", copyright = "(c) " .. input.name}
    end,
    ["scripture.book"] = function(input)
      if input.book == "john" then return {short_name = "Johannes"} end
    end,
    ["scripture.passage"] = function(input)
      if input.verse == 1 then return nil end
      return {text = "<b>" .. input.name .. " " .. input.book .. " " .. input.chapter .. "," .. input.verse}
    end,
  }
)";

///
/// Script and scripture workflow of \p scripts, the functions by id of the script named "Script <id>". \p selected is
/// the scripture name of a former run.
/// \p stored is a scripture file of the scripture folder.
///
struct scripture_fixture final
{
  test_utils::temp_folder folder;
  std::shared_ptr<workflow_script> script;
  std::shared_ptr<workflow_scripture> scripture;

  explicit scripture_fixture(
    std::string_view name,
    const std::map<std::string, std::string>& scripts,
    const std::optional<std::string>& selected = std::nullopt,
    const std::optional<std::filesystem::path>& stored = std::nullopt
  );
};

///
///
scripture_fixture::scripture_fixture(
  const std::string_view name,
  const std::map<std::string, std::string>& scripts,
  const std::optional<std::string>& selected,
  const std::optional<std::filesystem::path>& stored
)
  : folder{name}
{
  if(stored)
  {
    // The default scripture folder of the data folder
    std::filesystem::create_directories(folder.path() / workflow_scripture_settings::default_folder_name);
    std::filesystem::copy_file(*stored, folder.path() / workflow_scripture_settings::default_folder_name / stored->filename());
  }
  const auto scripts_folder = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts_folder);
  std::ranges::for_each(
    scripts,
    [&](const auto& entry)
    {
      test_utils::write_file(
        scripts_folder / (entry.first + ".lua"),
        std::format("return {{id = '{0}', name = 'Script {0}', functions = (function() {1} end)()}}", entry.first, entry.second)
      );
    }
  );
  if(selected)
  {
    const auto writer = std::make_shared<workflow_settings>(folder.path().string());
    std::ignore = workflow_scripture_settings{writer}.scripture_name->value(selected);
  }
  const auto settings = test_utils::make_script_settings(folder.path(), true, scripts_folder);
  script = std::make_shared<workflow_script>(settings);
  scripture = std::make_shared<workflow_scripture>(settings, script);
  // Last, as the app does
  test_utils::load_scripts(*script);
}

[[nodiscard]] auto john(const int verse) -> bible::reference
{
  return bible::reference::create_unguarded(bible::book_id::john, 3, verse);
}

///
/// \return a scripture file of the local test scriptures, std::nullopt if there are none
///
[[nodiscard]] auto stored_scripture() -> std::optional<std::filesystem::path>
{
  auto error = std::error_code{};
  for(const auto& entry : std::filesystem::directory_iterator{BIBSTD_TEST_SCRIPTURE_DIR, error})
  {
    if(entry.path().extension() == ".zip")
    {
      return entry.path();
    }
  }
  return std::nullopt;
}

} // namespace

TEST_CASE("workflow_scripture_offers_the_scriptures_of_a_script", "[workflow]")
{
  auto fixture = scripture_fixture{"workflow_scripture_offers_the_scriptures_of_a_script", {{"test", scripture_script}}};
  const auto& scripture = *fixture.scripture;
  REQUIRE(test_utils::wait_until([&]() { return scripture.settings().scripture_name->value() == "AAA (test)"; }));
  CHECK(scripture.scripture_count() == 2);
  // Named after the id of the script, the script is asked with its own names. The first scripture is selected.
  CHECK(scripture.settings().scripture_name->value() == "AAA (test)");

  const auto information = scripture.information({{"BBB (test)"}});
  REQUIRE(information);
  CHECK(
    *information == bible::scripture_info{.name = "BBB (test)", .abbreviation = "BBB", .language = "de", .copyright = "(c) BBB"}
  );
  CHECK_FALSE(scripture.information({{"BBB"}}));
  CHECK(scripture.book_information({{}}, bible::book_id::john) == bible::book_name{.short_name = "Johannes"});
  CHECK_FALSE(scripture.book_information({{}}, bible::book_id::mark));

  const auto passage = scripture.passage({
    {.reference = john(16), .scripture_name = std::nullopt}
  });
  REQUIRE(passage);
  // Markup of a web page shows as text
  CHECK(passage->passage.content == R"(<p data-id="undefined">&lt;b&gt;AAA john 3,16</p>)");
  CHECK_FALSE(scripture.passage({
    {.reference = john(1), .scripture_name = std::nullopt}
  }));

  // No scripture object, the fallback versification is used
  CHECK(
    scripture.versification_or_fallback({{}}).get() ==
    workflow_scripture::default_versifications.at(scripture.settings().fallback_versification->value())
  );
  // A name the script does not offer
  CHECK_FALSE(scripture.information({{"CCC"}}));
  CHECK_FALSE(scripture.passage({
    {.reference = john(16), .scripture_name = "CCC"}
  }));
}

TEST_CASE("workflow_scripture_keeps_the_name_until_the_scripts_are_loaded", "[workflow]")
{
  auto fixture = scripture_fixture{
    "workflow_scripture_keeps_the_name_until_the_scripts_are_loaded", {{"test", scripture_script}}, "BBB (test)"
  };
  // The scripts load while the workflows are constructed, the name of a former run survives it
  REQUIRE(test_utils::wait_until([&]() { return fixture.scripture->scripture_count() == 2; }));
  CHECK(fixture.scripture->settings().scripture_name->value() == "BBB (test)");
  CHECK(fixture.scripture->information({{}}));
}

TEST_CASE("workflow_scripture_keeps_the_name_of_a_script_next_to_stored_scriptures", "[workflow]")
{
  const auto stored = stored_scripture();
  if(!stored)
  {
    // The scriptures are not part of the repository, \see bibstd_test/res/scripture/README.md.
    SKIP(std::format("no scriptures in {}", BIBSTD_TEST_SCRIPTURE_DIR));
  }

  SECTION("offered by the script")
  {
    auto fixture = scripture_fixture{
      "workflow_scripture_keeps_the_name_of_a_script_offered", {{"test", scripture_script}}, "BBB (test)", stored
    };
    // Kept while the scripts load, although the stored scripture is known already, and after
    CHECK(fixture.scripture->settings().scripture_name->value() == "BBB (test)");
    REQUIRE(test_utils::wait_until([&]() { return fixture.scripture->scripture_count() == 3; }));
    CHECK(fixture.scripture->settings().scripture_name->value() == "BBB (test)");
  }

  SECTION("gone from the script")
  {
    auto fixture = scripture_fixture{
      "workflow_scripture_keeps_the_name_of_a_script_gone", {{"test", scripture_script}}, "CCC (test)", stored
    };
    // Pointed at the stored scripture once the scripts are loaded
    const auto& name = fixture.scripture->settings().scripture_name;
    CHECK(test_utils::wait_until([&]() { return name->value() && !name->value()->ends_with("(test)"); }));
  }
}

TEST_CASE("workflow_scripture_offers_the_scriptures_of_all_scripts", "[workflow]")
{
  auto fixture = scripture_fixture{
    "workflow_scripture_offers_the_scriptures_of_all_scripts",
    {
      {"a_partial", "return {['scripture.names'] = function() return {names = {'XXX'}} end}"},
      {"b_complete", scripture_script},
      {"c_complete", scripture_script},
      }
  };
  const auto& scripture = *fixture.scripture;
  // Of the scripts offering all manifests, the same names told apart by the script
  REQUIRE(test_utils::wait_until([&]() { return scripture.scripture_count() == 4; }));
  const auto information = scripture.information({{"BBB (c_complete)"}});
  REQUIRE(information);
  CHECK(information->name == "BBB (c_complete)");
  CHECK(scripture.information({{"AAA (b_complete)"}}));
  CHECK_FALSE(scripture.information({{"XXX (a_partial)"}}));
}

TEST_CASE("workflow_scripture_follows_the_scripts_loaded_anew", "[workflow]")
{
  auto fixture = scripture_fixture{"workflow_scripture_follows_the_scripts_loaded_anew", {{"first", scripture_script}}};
  const auto& scripture = *fixture.scripture;
  const auto scripts = fixture.folder.path() / "user_scripts";
  REQUIRE(test_utils::wait_until([&]() { return scripture.settings().scripture_name->value() == "AAA (first)"; }));
  const auto changed = [&]()
  {
    auto count = std::make_shared<std::atomic<int>>(0);
    return std::pair{count, scripture.connect(&workflow_scripture_sigs::scriptures_changed, [count]() { ++*count; })};
  }();

  // While the app runs: a script added, its scriptures join the others and the selected one stays
  test_utils::write_file(
    scripts / "added.lua",
    "return {id = 'second', name = 'Second', functions = {['scripture.names'] = function() return {names = {'CCC'}} end, "
    "['scripture.information'] = function(input) return {abbreviation = input.name} end, "
    "['scripture.book'] = function() end, ['scripture.passage'] = function() return {text = 'new'} end}}"
  );
  test_utils::load_scripts(*fixture.script);
  REQUIRE(test_utils::wait_until([&]() { return scripture.scripture_count() == 3; }));
  CHECK(scripture.settings().scripture_name->value() == "AAA (first)");
  CHECK(scripture.information({{"CCC (second)"}}));
  CHECK(scripture.information({{"BBB (first)"}}));

  // A script removed, its scriptures are gone and the selected one moves to one that is left
  std::filesystem::remove(scripts / "first.lua");
  test_utils::load_scripts(*fixture.script);
  REQUIRE(test_utils::wait_until([&]() { return scripture.scripture_count() == 1; }));
  CHECK(test_utils::wait_until([&]() { return scripture.settings().scripture_name->value() == "CCC (second)"; }));
  CHECK_FALSE(scripture.information({{"AAA (first)"}}));
  const auto passage = scripture.passage({
    {.reference = john(16), .scripture_name = std::nullopt}
  });
  REQUIRE(passage);
  CHECK(passage->passage.content.contains("new"));
  // The user interface is told each time
  CHECK(test_utils::wait_until([&]() { return *changed.first >= 2; }));
}

TEST_CASE("workflow_scripture_changes_the_name_setting_without_its_lock", "[workflow]")
{
  auto fixture =
    scripture_fixture{"workflow_scripture_changes_the_name_setting_without_its_lock", {{"first", scripture_script}}};
  const auto& scripture = *fixture.scripture;
  REQUIRE(test_utils::wait_until([&]() { return scripture.settings().scripture_name->value() == "AAA (first)"; }));
  // Listeners called right where the setting changes, each calls back into the workflow
  auto counted = std::make_shared<std::atomic<int>>(0);
  const auto count = [&scripture, counted]() { *counted += static_cast<int>(scripture.scripture_count()) + 1; };
  const auto value = scripture.settings().scripture_name->connect(&framework::setting_signals::value_changed, count);
  const auto validator = scripture.settings().scripture_name->connect(&framework::setting_signals::validator_changed, count);

  // The selected scripture goes with its script, so the names and the name change
  std::filesystem::remove(fixture.folder.path() / "user_scripts" / "first.lua");
  test_utils::write_file(
    fixture.folder.path() / "user_scripts" / "second.lua",
    std::format("return {{id = 'second', name = 'Second', functions = (function() {} end)()}}", scripture_script)
  );
  test_utils::load_scripts(*fixture.script);
  CHECK(test_utils::wait_until([&]() { return scripture.settings().scripture_name->value() == "AAA (second)"; }));
  CHECK(test_utils::wait_until([&]() { return *counted > 0; }));
}

TEST_CASE("workflow_scripture_passage_ends_on_stop", "[workflow]")
{
  auto fixture = scripture_fixture{
    "workflow_scripture_passage_ends_on_stop",
    {{"endless",
      "return {['scripture.names'] = function() return {names = {'AAA'}} end, "
      "['scripture.information'] = function() end, ['scripture.book'] = function() end, "
      "['scripture.passage'] = function() while true do end end}"}}
  };
  // The passage is of the selected scripture
  REQUIRE(test_utils::wait_until([&]() { return fixture.scripture->settings().scripture_name->value() == "AAA (endless)"; }));
  auto passage = std::async(
    std::launch::async,
    [&]()
    {
      return fixture.scripture->passage({
        {.reference = john(16), .scripture_name = std::nullopt}
      });
    }
  );
  REQUIRE(passage.wait_for(std::chrono::milliseconds{200}) == std::future_status::timeout);
  fixture.script->shutdown();
  REQUIRE(passage.wait_for(std::chrono::seconds{5}) == std::future_status::ready);
  CHECK_FALSE(passage.get());
}

} // namespace bibstd::workflow
