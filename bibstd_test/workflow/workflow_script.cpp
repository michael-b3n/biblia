#include "test_utils/files.hpp"
#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"

#include <bibstd/lua/script_table.hpp>
#include <bibstd/util/identifier.hpp>
#include <bibstd/workflow/workflow_base.hpp>
#include <bibstd/workflow/workflow_script.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <thread>
#include <vector>

namespace bibstd::workflow
{
namespace
{

// Manifest of the tests, its function reports what the script saw while loading
struct report_manifest final
{
  static inline const util::path id{"test.report"};
  using input = lua::script_table<>;
  using output = lua::script_table<lua::field<"value", std::string>>;
};

// Manifests a script offers or not
struct a_manifest final
{
  static inline const util::path id{"test.a"};
  using input = lua::script_table<>;
  using output = lua::script_table<>;
};

struct b_manifest final
{
  static inline const util::path id{"test.b"};
  using input = lua::script_table<>;
  using output = lua::script_table<>;
};

struct missing_manifest final
{
  static inline const util::path id{"test.missing"};
  using input = lua::script_table<>;
  using output = lua::script_table<>;
};

// Echoes its input, each type of value a script can take and return
using echo_table = lua::script_table<
  lua::field<"text", std::string>,
  lua::field<"count", std::int64_t>,
  lua::field<"ratio", double>,
  lua::field<"flag", bool>,
  lua::field<"note", std::optional<std::string>>,
  lua::field<"list", std::vector<std::string>>,
  lua::field<"scores", std::map<std::string, std::int64_t>>>;

struct echo_manifest final
{
  static inline const util::path id{"test.echo"};
  using input = echo_table;
  using output = echo_table;
};

///
/// \return Lua of the script \p id offering the function of report_manifest, it tells the value \p expression had
/// while loading
///
[[nodiscard]] auto report(const std::string& id, const std::string& expression) -> std::string
{
  return std::format(
    "local value = tostring({}) return {{id = '{}', name = 'Script {}', "
    "functions = {{['test.report'] = function() return {{value = value}} end}}}}",
    expression,
    id,
    id
  );
}

///
/// \return what the scripts offering report_manifest report, as "value (id)"
///
[[nodiscard]] auto reports(const workflow_script& workflow) -> std::vector<std::string>
{
  const auto scripts = workflow.scripts<report_manifest>();
  return scripts | std::views::keys |
         std::views::transform(
           [&](const util::identifier& script)
           {
             const auto output = workflow.run<report_manifest>(script, {});
             return std::format("{} ({})", output ? output->get<"value">() : std::string{"?"}, script.string());
           }
         ) |
         std::ranges::to<std::vector>();
}

///
/// \return the ids of \p scripts
///
[[nodiscard]] auto ids(const workflow_script::scripts_type& scripts) -> std::vector<std::string>
{
  return scripts | std::views::keys | std::views::transform(&util::identifier::string) | std::ranges::to<std::vector>();
}

///
/// \return the ids of the scripts the app ships, the ones loaded without the scripts of the user. \p folder is
/// the one of the test.
///
[[nodiscard]] auto bundled_ids(const test_utils::temp_folder& folder) -> std::vector<std::string>
{
  auto workflow = workflow_script{test_utils::make_script_settings(folder.path() / "bundled")};
  test_utils::load_scripts(workflow);
  return ids(workflow.scripts());
}

///
/// \return the ids of the scripts of \p workflow without the bundled ones
///
[[nodiscard]] auto user_ids(const workflow_script& workflow, const test_utils::temp_folder& folder) -> std::vector<std::string>
{
  const auto bundled = bundled_ids(folder);
  return ids(workflow.scripts()) | std::views::filter([&](const auto& id) { return !std::ranges::contains(bundled, id); }) |
         std::ranges::to<std::vector>();
}

///
/// Workflow registering to Lua the way workflow_template shows.
///
class workflow_test final : public workflow_base<void>
{
  // Variables
  const std::shared_ptr<workflow_script> workflow_script_;
  const std::string name_;

public: // Structors
  explicit workflow_test(std::shared_ptr<workflow_script> workflow_script, std::string name = "test");
  ~workflow_test() noexcept override;

public: // Variables
  bool registered{false};
  int value{42};

private: // Variables
  // Last, so it is taken back first
  lua::registration registrations_;
};

///
///
workflow_test::workflow_test(std::shared_ptr<workflow_script> workflow_script, std::string name)
  : workflow_script_{std::move(workflow_script)}
  , name_{std::move(name)}
{
  auto nested = workflow_script_->register_function(name_ + ".nested.value", [this]() { return value; });
  auto twice = workflow_script_->register_function(name_ + ".twice", [this]() { return 2 * value; });
  registered = nested && twice;
  registrations_ << std::move(nested) << std::move(twice);
}

///
///
workflow_test::~workflow_test() noexcept = default;

} // namespace

TEST_CASE("workflow_script_sets_up_the_state", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_sets_up_the_state"};
  auto workflow = workflow_script{test_utils::make_script_settings(folder.path())};
  CHECK(workflow.state()->script("return type(root.interface.util) == 'table' and util == nil and io == nil").get<bool>());
  // Disabled, so loaded without a scripts folder
  test_utils::load_scripts(workflow);
  CHECK_FALSE(std::filesystem::exists(folder.path() / "scripts"));
}

TEST_CASE("workflow_script_registers_a_workflow", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_registers_a_workflow"};
  const auto script = std::make_shared<workflow_script>(test_utils::make_script_settings(folder.path()));
  const auto workflow = std::make_shared<workflow_test>(script);
  REQUIRE(workflow->registered);
  workflow->value = 7;
  CHECK(
    script->state()
      ->script("return root.interface.workflow.test.nested.value() + root.interface.workflow.test.twice()")
      .get<int>() == 21
  );
}

TEST_CASE("workflow_script_drops_the_functions_of_a_destroyed_workflow", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_drops_the_functions_of_a_destroyed_workflow"};
  const auto script = std::make_shared<workflow_script>(test_utils::make_script_settings(folder.path()));
  const auto other = workflow_test{script, "other"};
  {
    const auto workflow = workflow_test{script};
    REQUIRE(script->state().run_script("before.lua", "held = workflow.test.twice return held()"));
  }
  // Its functions only: no script reaches the destroyed workflow, the other one and the scripts go on
  CHECK(script->state().run_script("after.lua", "return workflow.test == nil and workflow.other.twice() == 84"));
  CHECK_FALSE(script->state().run_script("after.lua", "return workflow.test.twice()"));
  CHECK(script->state().run_script("after.lua", "return 1"));
}

TEST_CASE("workflow_script_rejects_a_taken_name", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_rejects_a_taken_name"};
  const auto script = std::make_shared<workflow_script>(test_utils::make_script_settings(folder.path()));
  const auto first = workflow_test{script, "shared"};
  const auto second = workflow_test{script, "shared"};
  CHECK(first.registered);
  CHECK_FALSE(second.registered);
  CHECK(script->state()->script("return root.interface.workflow.shared.nested.value()").get<int>() == 42);
}

TEST_CASE("workflow_script_loads_the_user_scripts", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_loads_the_user_scripts"};
  const auto scripts = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts);
  // Each script reports what it sees
  test_utils::write_file(scripts / "a.lua", "shared = 'a' " + report("a", "shared"));
  // Globals of other scripts are out of reach
  test_utils::write_file(scripts / "b.lua", report("b", "shared"));
  // Failing scripts leave the others running
  test_utils::write_file(scripts / "c.lua", "error('broken')");
  test_utils::write_file(scripts / "e.lua", report("e", "'ran'"));
  test_utils::write_file(scripts / "f.txt", report("f", "'ran'"));

  auto workflow = workflow_script{test_utils::make_script_settings(folder.path(), true, scripts)};
  test_utils::load_scripts(workflow);
  CHECK(reports(workflow) == std::vector<std::string>{"a (a)", "nil (b)", "ran (e)"});
  CHECK(workflow.state()->script("return shared == nil").get<bool>());
}

TEST_CASE("workflow_script_loads_the_scripts_anew", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_loads_the_scripts_anew"};
  const auto scripts = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(scripts / "a.lua", report("a", "'first'"));
  test_utils::write_file(scripts / "b.lua", report("b", "'b'"));
  auto workflow = workflow_script{test_utils::make_script_settings(folder.path(), true, scripts)};
  test_utils::load_scripts(workflow);
  CHECK(reports(workflow) == std::vector<std::string>{"first (a)", "b (b)"});

  // Changed and removed scripts, e.g. loaded again from the settings
  test_utils::write_file(scripts / "a.lua", report("a", "'second'"));
  std::filesystem::remove(scripts / "b.lua");
  test_utils::load_scripts(workflow);
  CHECK(reports(workflow) == std::vector<std::string>{"second (a)"});
}

TEST_CASE("workflow_script_ends_loading_once_destroyed", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_ends_loading_once_destroyed"};
  const auto scripts = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(scripts / "a.lua", "while true do end");
  test_utils::write_file(scripts / "b.lua", "return {}");

  auto workflow = std::make_unique<workflow_script>(test_utils::make_script_settings(folder.path(), true, scripts));
  auto loaded = std::atomic<bool>{false};
  const auto connection = workflow->connect(&workflow_script_sigs::scripts_loaded, [&]() { loaded = true; });
  workflow->load_scripts();
  std::this_thread::sleep_for(std::chrono::milliseconds{100});
  REQUIRE_FALSE(loaded);
  // The endless script is stopped and the loader joins
  auto destroyed = std::async(std::launch::async, [&]() { workflow.reset(); });
  CHECK(destroyed.wait_for(std::chrono::seconds{5}) == std::future_status::ready);
}

TEST_CASE("workflow_script_loads_no_bytecode", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_loads_no_bytecode"};
  const auto scripts = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts);
  {
    const auto workflow = workflow_script{test_utils::make_script_settings(folder.path())};
    const auto bytecode = "return string.dump(function() " + report("a", "'ran'") + " end)";
    test_utils::write_file(scripts / "a.lua", workflow.state()->script(bytecode).get<std::string>());
  }
  test_utils::write_file(
    scripts / "b.lua", report("b", "load(string.dump(function() end)) == nil and dofile == nil and loadfile == nil")
  );

  auto workflow = workflow_script{test_utils::make_script_settings(folder.path(), true, scripts)};
  test_utils::load_scripts(workflow);
  CHECK(reports(workflow) == std::vector<std::string>{"true (b)"});
}

TEST_CASE("workflow_script_creates_a_missing_folder", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_creates_a_missing_folder"};
  auto workflow = workflow_script{test_utils::make_script_settings(folder.path(), true)};
  REQUIRE_FALSE(std::filesystem::exists(folder.path() / "scripts"));
  test_utils::load_scripts(workflow);
  // Empty, for the user to put scripts into
  CHECK(std::filesystem::is_empty(folder.path() / "scripts"));
  CHECK(user_ids(workflow, folder).empty());
}

TEST_CASE("workflow_script_loads_the_bundled_scripts", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_loads_the_bundled_scripts"};
  const auto scripts = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(scripts / "a.lua", report("a", "'ran'"));
  // The id of a bundled script is taken
  test_utils::write_file(scripts / "b.lua", report("lookup_bibleserver", "'ran'"));

  SECTION("without the scripts of the user")
  {
    auto workflow = workflow_script{test_utils::make_script_settings(folder.path(), false, scripts)};
    test_utils::load_scripts(workflow);
    CHECK(std::ranges::contains(ids(workflow.scripts()), std::string{"lookup_bibleserver"}));
    CHECK(reports(workflow).empty());
  }

  SECTION("before the scripts of the user")
  {
    auto workflow = workflow_script{test_utils::make_script_settings(folder.path(), true, scripts)};
    test_utils::load_scripts(workflow);
    CHECK(user_ids(workflow, folder) == std::vector<std::string>{"a"});
    CHECK(reports(workflow) == std::vector<std::string>{"ran (a)"});
  }
}

TEST_CASE("workflow_script_offers_the_scripts_of_manifests", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_offers_the_scripts_of_manifests"};
  const auto scripts = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(
    scripts / "both.lua",
    "return {id = 'both', name = 'Both', functions = {['test.a'] = function() end, ['test.b'] = function() end}}"
  );
  // Only named functions count
  test_utils::write_file(
    scripts / "one.lua",
    "return {id = 'one', name = 'One', functions = {['test.a'] = function() end, ['test.b'] = 1, function() end}}"
  );
  // A script returning no table offers nothing
  test_utils::write_file(scripts / "none.lua", "return 1");

  auto workflow = workflow_script{test_utils::make_script_settings(folder.path(), true, scripts)};
  test_utils::load_scripts(workflow);
  CHECK(ids(workflow.scripts<a_manifest>()) == std::vector<std::string>{"both", "one"});
  CHECK(ids(workflow.scripts<a_manifest, b_manifest>()) == std::vector<std::string>{"both"});
  CHECK(workflow.scripts<missing_manifest>().empty());
  // All of them without a manifest
  CHECK(user_ids(workflow, folder) == std::vector<std::string>{"both", "one"});
}

TEST_CASE("workflow_script_knows_a_script_by_its_id", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_knows_a_script_by_its_id"};
  const auto scripts = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts);
  const auto script = [](const std::string& description)
  { return "return {" + description + " functions = {['test.report'] = function() return {value = 'ran'} end}}"; };
  // The name of the file plays no role
  test_utils::write_file(scripts / "1.lua", script("id = 'zeta', name = 'Zeta Script',"));
  test_utils::write_file(scripts / "2.lua", script("id = 'Alpha_2', name = 'Alpha',"));
  // The first one keeps a taken id
  test_utils::write_file(scripts / "3.lua", script("id = 'zeta', name = 'Second',"));
  // Other characters of an id are replaced
  test_utils::write_file(scripts / "4.lua", script("id = 'has space', name = 'Space',"));
  // No id, no name
  test_utils::write_file(scripts / "5.lua", script("id = 5, name = 'Number',"));
  test_utils::write_file(scripts / "50.lua", script("id = '', name = 'Empty id',"));
  test_utils::write_file(scripts / "6.lua", script("name = 'No id',"));
  test_utils::write_file(scripts / "7.lua", script("id = 'no_name',"));
  test_utils::write_file(scripts / "8.lua", script("id = 'empty_name', name = '',"));
  // No functions
  test_utils::write_file(scripts / "9.lua", "return {id = 'no_functions', name = 'No functions'}");

  auto workflow = workflow_script{test_utils::make_script_settings(folder.path(), true, scripts)};
  test_utils::load_scripts(workflow);
  const auto loaded = workflow.scripts();
  REQUIRE(user_ids(workflow, folder) == std::vector<std::string>{"Alpha_2", "has_space", "zeta"});
  CHECK(loaded.at(util::identifier{"zeta"}).name == "Zeta Script");
  CHECK(loaded.at(util::identifier{"Alpha_2"}).name == "Alpha");
  CHECK(reports(workflow) == std::vector<std::string>{"ran (Alpha_2)", "ran (has_space)", "ran (zeta)"});
  // Neither by the name of its file
  CHECK_FALSE(workflow.run<report_manifest>(util::identifier{"1"}, {}));
}

TEST_CASE("workflow_script_runs_a_function", "[workflow]")
{
  const auto folder = test_utils::temp_folder{"workflow_script_runs_a_function"};
  const auto scripts = folder.path() / "user_scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(
    scripts / "echo.lua",
    R"(
      return { id = "echo", name = "Echo", functions = {
        ["test.echo"] = function(input)
          local case = input.text
          if case == "broken" then error("broken") end
          if case == "no note" then input.note = nil end
          if case == "integral float" then input.count = 3.0 end
          if case == "fraction" then input.count = 1.5 end
          if case == "unknown key" then input.other = "x" end
          if case == "number in list" then input.list = {"a", 1} end
          if case == "number as key" then input.scores = {2} end
          input.text = "echo " .. case
          input.ratio = input.ratio * 2
          input.flag = not input.flag
          return input
        end,
      } }
    )"
  );
  auto workflow = workflow_script{test_utils::make_script_settings(folder.path(), true, scripts)};
  test_utils::load_scripts(workflow);
  const auto input = [](const std::string& text)
  {
    return echo_table{
      text, 1, 0.5, true, std::string{"note"},
          {"a", "b"},
          {{"x", 1}}
    };
  };
  const auto run = [&](const std::string& text) { return workflow.run<echo_manifest>(util::identifier{"echo"}, input(text)); };

  CHECK(
    run("john3") == echo_table{
                      "echo john3", 1, 1.0, false, std::string{"note"},
                          {"a", "b"},
                          {{"x", 1}}
  }
  );
  // nil is no value of an optional
  const auto no_note = run("no note");
  REQUIRE(no_note);
  CHECK(no_note->get<"note">() == std::nullopt);
  // A float is no integer, even of an integral value
  CHECK_FALSE(run("integral float"));
  // Output not fitting the manifest, a failing function
  CHECK_FALSE(run("fraction"));
  CHECK_FALSE(run("unknown key"));
  CHECK_FALSE(run("number in list"));
  CHECK_FALSE(run("number as key"));
  CHECK_FALSE(run("broken"));
  // A missing script or function
  CHECK_FALSE(workflow.run<echo_manifest>(util::identifier{"missing"}, input("john3")));
  CHECK_FALSE(workflow.run<missing_manifest>(util::identifier{"echo"}, {}));
}

} // namespace bibstd::workflow
