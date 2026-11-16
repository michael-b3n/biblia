#pragma once

#include "test_utils/files.hpp"
#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/lua/lua_engine.hpp>
#include <arcstd/lua/value.hpp>
#include <arcstd/lua/value_cast.hpp>
#include <bibstd/lua/bundled_scripts.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <format>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace bibstd::test_utils
{

///
/// Lua engine with the bundled scripts and the user scripts \p scripts by
/// file name, loaded. Lua expressions see the globals of scripts and \p texts.
///
struct script_sandbox final
{
  arcstd::test_utils::temp_folder folder;
  std::shared_ptr<arcstd::lua::lua_engine> script;
  std::map<std::string, std::string> texts;

  explicit script_sandbox(std::string_view name, const std::map<std::string, std::string>& scripts = {});

  ///
  /// \return the value of the Lua expression \p code
  ///
  [[nodiscard]] auto value(const std::string& code) const -> arcstd::lua::value;

  ///
  /// \return the text the Lua expression \p code gives
  ///
  [[nodiscard]] auto text(const std::string& code) const -> std::string;

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
    arcstd::test_utils::write_file(folder_of_scripts / file, code);
  }
  script =
    std::make_shared<arcstd::lua::lua_engine>(arcstd::test_utils::make_script_settings(folder.path(), true), lua::bundled::all);
  arcstd::test_utils::load_scripts(*script);
}

///
///
inline auto script_sandbox::value(const std::string& code) const -> arcstd::lua::value
{
  // Each text a local of the expression, in a long string no text closes
  auto prelude = std::string{};
  for(const auto& [name, text] : texts)
  {
    auto level = std::string{};
    while(text.contains("]" + level + "]"))
    {
      level += "=";
    }
    // Lua drops a line break right after the opening bracket, so the text starts after one
    prelude += std::format("local {} = [{}[\n{}]{}]\n", name, level, text, level);
  }
  const auto result = script->run_script("test.lua", prelude + "return " + code);
  REQUIRE(result);
  return *result;
}

///
///
inline auto script_sandbox::text(const std::string& code) const -> std::string
{
  const auto result = arcstd::lua::from_value<std::string>(value(code));
  REQUIRE(result);
  return *result;
}

///
///
inline auto script_sandbox::check(const std::string& code) const -> bool
{
  const auto result = arcstd::lua::from_value<bool>(value(code));
  REQUIRE(result);
  return *result;
}

} // namespace bibstd::test_utils
