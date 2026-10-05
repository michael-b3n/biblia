#include "test_utils/files.hpp"
#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"
#include "test_utils/wait_until.hpp"

#include <bibstd/system/locale.hpp>
#include <bibstd/workflow/workflow_bible_ref_lookup.hpp>
#include <bibstd/workflow/workflow_script.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace bibstd::workflow
{
namespace
{

// Offers the translations AAA and BBB
constexpr auto lookup_script = R"(
  return {
    id = "mine",
    name = "Mine",
    functions = {
      ["lookup.translations"] = function(input)
        return {names = {"AAA", "BBB"}, defaults = {"BBB", "CCC"}}
      end,
      ["lookup.url"] = function() return {url = "https://example.com"} end,
    },
  }
)";

// Offers no translations
constexpr auto plain_script = R"(
  return {
    id = "plain",
    name = "Plain",
    functions = {
      ["lookup.translations"] = function() return {names = {}} end,
      ["lookup.url"] = function() return {url = "https://example.com"} end,
    },
  }
)";

///
/// Script and lookup workflow with the user scripts \p scripts by file name. \p script and \p translations are the
/// settings of a former run.
///
struct lookup_fixture final
{
  test_utils::temp_folder folder;
  std::shared_ptr<workflow_script> script;
  std::shared_ptr<workflow_bible_ref_lookup> lookup;

  explicit lookup_fixture(
    std::string_view name,
    const std::map<std::string, std::string>& scripts = {},
    const std::string& selected = {},
    const std::vector<std::string>& translations = {}
  );
};

///
///
lookup_fixture::lookup_fixture(
  const std::string_view name,
  const std::map<std::string, std::string>& scripts,
  const std::string& selected,
  const std::vector<std::string>& translations
)
  : folder{name}
{
  const auto scripts_folder = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts_folder);
  std::ranges::for_each(
    scripts, [&](const auto& entry) { test_utils::write_file(scripts_folder / entry.first, entry.second); }
  );
  if(!selected.empty())
  {
    const auto writer = std::make_shared<workflow_settings>(folder.path().string());
    const auto settings = workflow_bible_ref_lookup_settings{writer};
    std::ignore = settings.script->value(selected);
    std::ignore = settings.translations->value(translations);
  }
  const auto settings = test_utils::make_script_settings(folder.path(), !scripts.empty(), scripts_folder);
  script = std::make_shared<workflow_script>(settings);
  lookup = std::make_shared<workflow_bible_ref_lookup>(settings, script);
  // Last, as the app does
  test_utils::load_scripts(*script);
}

///
/// \return the values \p setting can be chosen from
///
template<typename T>
[[nodiscard]] auto available(const T& setting)
{
  using value_type = std::remove_cvref_t<decltype(setting->value())>;
  return std::get<typename framework::setting_validator_list<value_type>::sptr_type>(setting->validator)->available();
}

///
/// \return the translations bibleserver.com shows by default, by the language of the user
///
[[nodiscard]] auto default_translations() -> std::vector<std::string>
{
  using result_type = std::vector<std::string>;
  return system::locale::preferred_language() == util::language::german ? result_type{"NGÜ", "ELB"} : result_type{"NIV", "ESV"};
}

} // namespace

TEST_CASE("workflow_bible_ref_lookup_chooses_the_bundled_script", "[workflow]")
{
  const auto fixture = lookup_fixture{"workflow_bible_ref_lookup_chooses_the_bundled_script"};
  const auto& settings = fixture.lookup->settings();
  // Loaded although the scripts of the user are not, with the translations of the language
  REQUIRE(test_utils::wait_until([&]() { return settings.translations->value() == default_translations(); }));
  CHECK(settings.script->value() == "lookup_bibleserver");
  CHECK(available(settings.script) == std::vector<std::string>{"lookup_bible_com", "lookup_bibleserver"});
  CHECK(available(settings.translations).size() == 17);
}

TEST_CASE("workflow_bible_ref_lookup_follows_the_chosen_script", "[workflow]")
{
  const auto fixture = lookup_fixture{
    "workflow_bible_ref_lookup_follows_the_chosen_script", {{"mine.lua", lookup_script}, {"other.lua", "return 1"}}
  };
  const auto& settings = fixture.lookup->settings();
  REQUIRE(test_utils::wait_until([&]() { return settings.translations->value() == default_translations(); }));
  // Only scripts offering the lookup, the chosen one stays
  CHECK(available(settings.script) == std::vector<std::string>{"lookup_bible_com", "lookup_bibleserver", "mine"});
  CHECK(settings.script->value() == "lookup_bibleserver");

  // Another script has other translations, the defaults it offers are chosen
  REQUIRE(settings.script->value("mine"));
  REQUIRE(test_utils::wait_until([&]() { return settings.translations->value() == std::vector<std::string>{"BBB"}; }));
  CHECK(available(settings.translations) == std::vector<std::string>{"AAA", "BBB"});
  // The other bundled script, with the translation of the language
  const auto german = system::locale::preferred_language() == util::language::german;
  REQUIRE(settings.script->value("lookup_bible_com"));
  REQUIRE(test_utils::wait_until([&]() { return available(settings.translations).size() == 12; }));
  CHECK(
    test_utils::wait_until([&]() { return settings.translations->value() == std::vector<std::string>{german ? "ELB" : "NIV"}; })
  );
  REQUIRE(settings.script->value("mine"));
  REQUIRE(test_utils::wait_until([&]() { return settings.translations->value() == std::vector<std::string>{"BBB"}; }));

  // The script is gone, the lookup is the one of the default script again, not of the first one
  std::filesystem::remove(fixture.folder.path() / "user_scripts" / "mine.lua");
  test_utils::load_scripts(*fixture.script);
  REQUIRE(test_utils::wait_until([&]() { return settings.script->value() == "lookup_bibleserver"; }));
  CHECK(test_utils::wait_until([&]() { return settings.translations->value() == default_translations(); }));
}

TEST_CASE("workflow_bible_ref_lookup_keeps_the_settings_of_a_former_run", "[workflow]")
{
  SECTION("of a script of the user")
  {
    const auto fixture = lookup_fixture{
      "workflow_bible_ref_lookup_keeps_the_settings_of_a_script", {{"mine.lua", lookup_script}}, "mine", {"AAA"}
    };
    const auto& settings = fixture.lookup->settings();
    REQUIRE(test_utils::wait_until([&]() { return available(settings.translations).size() == 2; }));
    CHECK(settings.script->value() == "mine");
    CHECK(settings.translations->value() == std::vector<std::string>{"AAA"});
  }

  SECTION("but no translation the script does not offer")
  {
    // The names of the translations before the scripts
    const auto former = std::vector<std::string>{"ngu", "LUT", "elb"};
    const auto fixture =
      lookup_fixture{"workflow_bible_ref_lookup_keeps_no_unknown_translation", {}, "lookup_bibleserver", former};
    const auto& settings = fixture.lookup->settings();
    REQUIRE(test_utils::wait_until([&]() { return !available(settings.translations).empty(); }));
    CHECK(test_utils::wait_until([&]() { return settings.translations->value() == std::vector<std::string>{"LUT"}; }));
  }

  SECTION("but no script that is gone")
  {
    const auto fixture = lookup_fixture{"workflow_bible_ref_lookup_keeps_no_script_gone", {}, "mine", {"AAA"}};
    const auto& settings = fixture.lookup->settings();
    REQUIRE(test_utils::wait_until([&]() { return settings.script->value() == "lookup_bibleserver"; }));
    CHECK(test_utils::wait_until([&]() { return settings.translations->value() == default_translations(); }));
  }
}

TEST_CASE("workflow_bible_ref_lookup_takes_a_script_without_translations", "[workflow]")
{
  const auto fixture = lookup_fixture{
    "workflow_bible_ref_lookup_takes_a_script_without_translations", {{"plain.lua", plain_script}}, "plain", {"NIV"}
  };
  const auto& settings = fixture.lookup->settings();
  // None to choose from, none is chosen
  REQUIRE(test_utils::wait_until([&]() { return settings.translations->value().empty(); }));
  CHECK(settings.script->value() == "plain");
  CHECK(available(settings.translations).empty());
}

} // namespace bibstd::workflow
