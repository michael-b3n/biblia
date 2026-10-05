#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"

#include <bibstd/workflow/workflow_script.hpp>
#include <bibstd/workflow/workflow_web.hpp>

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <memory>
#include <string>

namespace bibstd::workflow
{

TEST_CASE("workflow_web_fetches_web_pages", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_web_fetches_web_pages"};
  const auto script = std::make_shared<workflow_script>(test_utils::make_script_settings(folder.path()));
  const auto web = workflow_web{script};
  // Fails before any request, so no network is needed
  CHECK(web.fetch("not a url") == std::unexpected{std::string{"invalid_url"}});
  CHECK(
    script->state()
      ->script("local content, error = root.interface.workflow.web.fetch('not a url') return content == nil and error")
      .get<std::string>() == "invalid_url"
  );
}

TEST_CASE("workflow_web_takes_its_function_back_on_destruction", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_web_takes_its_function_back_on_destruction"};
  const auto script = std::make_shared<workflow_script>(test_utils::make_script_settings(folder.path()));
  {
    const auto web = workflow_web{script};
    CHECK(script->state()->script("return type(root.interface.workflow.web.fetch) == 'function'").get<bool>());
  }
  // Only it, the scripts go on
  CHECK(script->state()->script("return root.interface.workflow == nil").get<bool>());
  CHECK(script->state().run_script("after.lua", "return 1"));
}

} // namespace bibstd::workflow
