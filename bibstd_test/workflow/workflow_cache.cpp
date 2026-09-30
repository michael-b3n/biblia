#include "test_utils/script_settings.hpp"
#include "test_utils/temp_folder.hpp"

#include <bibstd/workflow/workflow_cache.hpp>
#include <bibstd/workflow/workflow_script.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace bibstd::workflow
{

TEST_CASE("workflow_cache_keeps_values_of_scripts", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_cache_keeps_values_of_scripts"};
  const auto run = [](const workflow_script& script, const std::string& code)
  { return script.state()->safe_script(code, sol::script_pass_on_error); };
  {
    const auto script = std::make_shared<workflow_script>(test_utils::make_script_settings(folder.path()));
    const auto cache = workflow_cache{script};
    CHECK(run(*script, "return root.interface.workflow.cache.get('a', 'key') == nil").get<bool>());
    run(
      *script,
      "root.interface.workflow.cache.set('a', 'key', 'value a') root.interface.workflow.cache.set('b', 'key', 'value b')"
    );
    run(*script, "root.interface.workflow.cache.set('a', 'gone', 'x') root.interface.workflow.cache.set('a', 'gone', nil)");
    // A name reaching out of the folder is made one inside of it
    CHECK(run(*script, "root.interface.workflow.cache.set('../a', 'key', 'x')").valid());
    CHECK(run(*script, "return root.interface.workflow.cache.get('___a', 'key')").get<std::string>() == "x");
    CHECK_FALSE(run(*script, "root.interface.workflow.cache.get('', 'key')").valid());
  }
  // Kept beyond the run, apart for each name
  const auto script = std::make_shared<workflow_script>(test_utils::make_script_settings(folder.path()));
  auto cache = workflow_cache{script};
  CHECK(run(*script, "return root.interface.workflow.cache.get('a', 'key')").get<std::string>() == "value a");
  CHECK(run(*script, "return root.interface.workflow.cache.get('b', 'key')").get<std::string>() == "value b");
  CHECK(run(*script, "return root.interface.workflow.cache.get('a', 'gone') == nil").get<bool>());
  CHECK(cache.get(util::identifier{"a"}, "key") == "value a");
  CHECK(std::filesystem::exists(folder.path() / "scripts" / "cache" / "a.sqlite"));
  CHECK(std::filesystem::exists(folder.path() / "scripts" / "cache" / "___a.sqlite"));
  CHECK_FALSE(std::filesystem::exists(folder.path() / "scripts" / "a.sqlite"));
}

TEST_CASE("workflow_cache_shuts_the_scripts_down_on_destruction", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_cache_shuts_the_scripts_down_on_destruction"};
  const auto script = std::make_shared<workflow_script>(test_utils::make_script_settings(folder.path()));
  {
    const auto cache = workflow_cache{script};
    CHECK(script->state()->script("return type(root.interface.workflow.cache.get) == 'function'").get<bool>());
  }
  CHECK_FALSE(script->state().run_script("after.lua", "return 1"));
}

} // namespace bibstd::workflow
