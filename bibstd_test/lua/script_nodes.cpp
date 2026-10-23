#include "test_utils/script_sandbox.hpp"

#include <bibstd/util/path.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <set>

namespace bibstd::lua
{

TEST_CASE("script_nodes_are_offered_to_the_other_scripts", "[lua]")
{
  const auto fixture = test_utils::script_sandbox{
    "script_nodes_are_offered_to_the_other_scripts",
    {
      // A function named after a manifest and data
      {"a.lua", "return {name = 'Mine', node = 'mine', ['test.answer'] = function() end, answer = function() return 42 end}"},
      // The node of another script, a bundled one too
      {"b.lua", "return {name = 'Other', node = 'mine', answer = function() return 0 end}"},
      {"c.lua", "return {name = 'Another', node = 'bibleserver'}"},
      // Uses the node of another script
      {"d.lua", "return {name = 'User', node = 'user', ['test.use'] = function() return {value = scripts.mine.answer()} end}"},
      // No node
      {"e.lua", "return {name = 'Empty'}"},
      }
  };
  CHECK(fixture.check("scripts.mine.answer() == 42"));
  CHECK(fixture.check("scripts.bibleserver['lookup.url'] ~= nil"));
  CHECK_FALSE(fixture.script->scripts().contains("Other"));
  CHECK_FALSE(fixture.script->scripts().contains("Another"));
  CHECK_FALSE(fixture.script->scripts().contains("Empty"));
  // Only the functions named after a manifest are the script's functions
  CHECK(fixture.script->scripts().at("Mine") == std::set<util::path>{util::path{"test.answer"}});
  // Read-only for scripts
  CHECK(fixture.check("not pcall(function() scripts.mine.answer = nil end)"));

  // Loaded anew, the node of a script gone is gone, the other one takes it
  std::filesystem::remove(fixture.folder.path() / "scripts" / "a.lua");
  test_utils::load_scripts(*fixture.script);
  CHECK(fixture.check("scripts.mine.answer() == 0"));
}

} // namespace bibstd::lua
