#pragma once

#include "test_utils/files.hpp"
#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"

#include <bibstd/workflow/workflow_script.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace bibstd::test_utils
{

///
/// Script workflow with the bundled scripts and the user scripts \p scripts by
/// file name, loaded. Lua expressions see the globals of scripts and \p texts.
///
struct script_sandbox final
{
  temp_folder folder;
  std::shared_ptr<workflow::workflow_script> script;
  std::map<std::string, std::string> texts;

  explicit script_sandbox(std::string_view name, const std::map<std::string, std::string>& scripts = {});

  ///
  /// \return the value of the Lua expression \p code
  ///
  [[nodiscard]] auto value(const std::string& code) const -> sol::object;

  ///
  /// \return true if the Lua expression \p code is true
  ///
  [[nodiscard]] auto check(const std::string& code) const -> bool;
};

///
///
inline script_sandbox::script_sandbox(const std::string_view name, const std::map<std::string, std::string>& scripts)
  : folder{name}
{
  const auto folder_of_scripts = folder.path() / "scripts";
  std::filesystem::create_directories(folder_of_scripts);
  for(const auto& [file, code] : scripts)
  {
    write_file(folder_of_scripts / file, code);
  }
  script = std::make_shared<workflow::workflow_script>(make_script_settings(folder.path(), true));
  load_scripts(*script);
}

///
///
inline auto script_sandbox::value(const std::string& code) const -> sol::object
{
  auto state = script->state();
  const auto run = state->load("return " + code, "test", sol::load_mode::text);
  REQUIRE(run.valid());
  auto function = run.get<sol::protected_function>();
  auto environment = sol::environment{*state, sol::create, state->traverse_get<sol::table>("root", "system", "sandbox")};
  for(const auto& [name, text] : texts)
  {
    environment[name] = text;
  }
  sol::set_environment(environment, function);
  const auto result = function();
  REQUIRE(result.valid());
  return result.get<sol::object>();
}

///
///
inline auto script_sandbox::check(const std::string& code) const -> bool
{
  return value(code).as<bool>();
}

} // namespace bibstd::test_utils
