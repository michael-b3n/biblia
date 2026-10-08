#include "test_utils/files.hpp"
#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"
#include "test_utils/wait_until.hpp"

#include <bibstd/workflow/workflow_cache.hpp>
#include <bibstd/workflow/workflow_script.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace bibstd::lua
{
namespace
{

///
/// Workflows running the example as installed, or activated.
///
struct example_fixture final
{
  test_utils::temp_folder folder;
  std::shared_ptr<workflow::workflow_script> script;
  std::shared_ptr<workflow::workflow_cache> cache;
  std::shared_ptr<workflow::workflow_scripture> scripture;

  example_fixture(std::string_view name, bool activate);
};

///
///
example_fixture::example_fixture(const std::string_view name, const bool activate)
  : folder{name}
{
  const auto scripts = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts);
  auto code = test_utils::read_file(std::filesystem::path{BIBSTD_TEST_LUA_EXAMPLES_DIR} / "example.lua");
  const auto inactive = std::string{"local enabled = false"};
  REQUIRE(code.contains(inactive));
  if(activate)
  {
    code.replace(code.find(inactive), inactive.size(), "local enabled = true");
  }
  test_utils::write_file(scripts / "example.lua", code);
  const auto settings = test_utils::make_script_settings(folder.path(), true, scripts);
  script = std::make_shared<workflow::workflow_script>(settings);
  cache = std::make_shared<workflow::workflow_cache>(script);
  scripture = std::make_shared<workflow::workflow_scripture>(settings, script);
  // Last, as the app does
  test_utils::load_scripts(*script);
}

} // namespace

TEST_CASE("example_script_is_inactive_as_installed", "[lua]")
{
  const auto fixture = example_fixture{"example_script_is_inactive_as_installed", false};
  CHECK(fixture.script->scripts<workflow::workflow_scripture::passage_manifest>().empty());
  CHECK(fixture.scripture->scripture_count() == 0);
}

TEST_CASE("example_script_provides_a_scripture", "[lua]")
{
  const auto fixture = example_fixture{"example_script_provides_a_scripture", true};
  const auto& scripture = *fixture.scripture;
  REQUIRE(test_utils::wait_until([&]() { return scripture.settings().scripture_name->value() == "DEMO (Example)"; }));
  CHECK(scripture.scripture_count() == 1);
  const auto information = scripture.information({{}});
  REQUIRE(information);
  CHECK(information->copyright == "Example script");
  CHECK(scripture.book_information({{}}, bible::book_id::john) == bible::book_name{.short_name = "John"});

  const auto passage = scripture.passage({
    {.reference = bible::reference::create_unguarded(bible::book_id::john, 3, 16), .scripture_name = std::nullopt}
  });
  REQUIRE(passage);
  CHECK(passage->passage.content == R"(<p data-id="undefined">Example text of john 3,16</p>)");
  // Kept in the cache of the script
  CHECK(std::filesystem::exists(fixture.folder.path() / "user_scripts" / "cache" / "example.sqlite"));
}

} // namespace bibstd::lua
