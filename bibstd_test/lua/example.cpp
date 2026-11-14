#include "test_utils/files.hpp"
#include "test_utils/scripts.hpp"
#include "test_utils/scripture_names.hpp"
#include "test_utils/temp_folder.hpp"
#include "test_utils/wait_until.hpp"

#include <arcstd/cache/cache_store.hpp>
#include <arcstd/lua/lua_engine.hpp>
#include <bibstd/lua/bundled_scripts.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::lua
{
namespace
{

///
/// Workflows running the example as installed, or activated.
///
struct example_fixture final
{
  arcstd::test_utils::temp_folder folder;
  std::shared_ptr<arcstd::lua::lua_engine> script;
  std::shared_ptr<arcstd::cache::cache_store> cache;
  std::shared_ptr<workflow::workflow_scripture> scripture;

  example_fixture(std::string_view name, bool activate);
};

///
///
example_fixture::example_fixture(const std::string_view name, const bool activate)
  : folder{name}
{
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  auto code = arcstd::test_utils::read_file(std::filesystem::path{BIBSTD_TEST_LUA_EXAMPLES_DIR} / "example.lua");
  const auto inactive = std::string{"local enabled = false"};
  REQUIRE(code.contains(inactive));
  if(activate)
  {
    code.replace(code.find(inactive), inactive.size(), "local enabled = true");
  }
  arcstd::test_utils::write_file(scripts / "example.lua", code);
  const auto settings = arcstd::test_utils::make_script_settings(folder.path(), true);
  script = std::make_shared<arcstd::lua::lua_engine>(settings, lua::bundled::all);
  cache = std::make_shared<arcstd::cache::cache_store>(settings->data_folder(), script);
  scripture = std::make_shared<workflow::workflow_scripture>(settings, script);
  // Last, as the app does
  arcstd::test_utils::load_scripts(*script);
}

} // namespace

TEST_CASE("example_script_is_inactive_as_installed", "[lua]")
{
  const auto fixture = example_fixture{"example_script_is_inactive_as_installed", false};
  CHECK_FALSE(fixture.script->scripts<workflow::workflow_scripture::passage_manifest>().contains("Example"));
  CHECK(test_utils::scripture_names_of(*fixture.scripture, "Example").empty());
}

TEST_CASE("example_script_provides_a_scripture", "[lua]")
{
  const auto fixture = example_fixture{"example_script_provides_a_scripture", true};
  const auto& scripture = *fixture.scripture;
  REQUIRE(arcstd::test_utils::wait_until([&]() { return !test_utils::scripture_names_of(scripture, "Example").empty(); }));
  CHECK(test_utils::scripture_names_of(scripture, "Example") == std::vector<std::string>{"DEMO (Example)"});
  const auto information = scripture.information({{"DEMO (Example)"}});
  REQUIRE(information);
  CHECK(information->copyright == "Example script");
  CHECK(scripture.book_information({{"DEMO (Example)"}}, bible::book_id::john) == bible::book_name{.short_name = "John"});

  const auto passage = scripture.passage({
    {.reference = bible::reference::create_unguarded(bible::book_id::john, 3, 16), .scripture_name = "DEMO (Example)"}
  });
  REQUIRE(passage);
  CHECK(passage->passage.xml() == R"(<p data-id="undefined">Example text of john 3,16</p>)");
  // Kept in the cache of the data folder
  CHECK(std::filesystem::exists(fixture.folder.path() / "cache" / "example.sqlite"));
}

} // namespace bibstd::lua
