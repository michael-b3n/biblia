#include "bibstd/lua/state_owner.hpp"
#include "bibstd/lua/function_cast.hpp"
#include "bibstd/lua/names.hpp"
#include "bibstd/lua/scripts.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/non_owning_ptr.hpp"

#include <atomic>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace bibstd::lua
{
namespace
{

///
/// \return true once root.system.shutdown_flag is set
///
[[nodiscard]] auto stop_requested(const util::non_owning_ptr<lua_State> lua) -> bool
{
  // On the stack only, the hook runs on every call
  const auto flag = sol::stack_table{lua, LUA_REGISTRYINDEX}.traverse_get<sol::optional<sol::light<const std::atomic<bool>>>>(
    LUA_RIDX_GLOBALS, names::node_root, names::node_system, names::value_shutdown_flag
  );
  return flag && flag->value()->load();
}

///
/// Hook failing the running code once the scripts are stopped, \see state_owner::shutdown
///
auto check_stop(const util::non_owning_ptr<lua_State> lua, [[maybe_unused]] const util::non_owning_ptr<lua_Debug> debug) -> void
{
  // luaL_error leaves without unwinding, so no C++ object may live here
  if(stop_requested(lua))
  {
    luaL_error(lua, "script stopped");
  }
}

///
/// \return the code of the embedded script \p file_name, nil if there is none, \see init.lua
///
[[nodiscard]] auto embedded(const std::string& file_name) -> std::optional<std::string>
{
  return internal::find(file_name).transform([](const auto& script) { return std::string{script.code}; });
}

} // namespace

///
///
state_owner::state_owner()
  : data_{std::make_shared<detail::state_data>()}
{
  // Often enough to stop in time, rare enough to cost nothing
  static constexpr auto stop_check_interval = 1'000'000;

  auto s = lock();

  // io, os, package and debug are left out, they would give scripts access to files and processes
  s->open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::utf8);

  auto system = s->create_table();
  system.set_function(names::function_embedded, function_cast(&embedded));
  system.set_function(names::function_log_debug, [](const std::string& text) { LOG_DEBUG("lua: {}", text); });
  system.set_function(names::function_log_info, [](const std::string& text) { LOG_INFO("lua: {}", text); });
  system.set_function(names::function_log_warning, [](const std::string& text) { LOG_WARN("lua: {}", text); });
  system.set_function(names::function_log_error, [](const std::string& text) { LOG_ERROR("lua: {}", text); });
  system[names::value_shutdown_flag] = sol::make_light(data_->shutdown_flag);
  (*s)[names::node_root] = s->create_table_with(names::node_interface, s->create_table(), names::node_system, system);

  const auto init = internal::find(names::embedded_script_init);
  if(!init)
  {
    throw util::exception{std::format("lua init failed: no embedded \"{}\"", names::embedded_script_init)};
  }
  // The file names of the other embedded scripts, init.lua runs them and each adds itself to the tree
  const auto files =
    internal::all() | std::views::filter([](const auto& script) { return script.name != names::embedded_script_init; }) |
    std::views::transform([](const auto& script) { return std::string{script.name}; }) | std::ranges::to<std::vector>();
  const auto chunk = s->load(init->code, std::format("@{}", init->name), sol::load_mode::text);
  if(!chunk.valid())
  {
    const sol::error error = chunk;
    throw util::exception{std::format("lua init failed: {}", error.what())};
  }
  if(const auto result = chunk.get<sol::protected_function>()(sol::as_table(files)); !result.valid())
  {
    const sol::error error = result;
    throw util::exception{std::format("lua init failed: {}", error.what())};
  }
  const auto sandbox = system.get<sol::optional<sol::table>>(names::node_sandbox);
  if(!sandbox)
  {
    throw util::exception{std::format("lua init failed: \"{}\" left no sandbox", names::embedded_script_init)};
  }
  data_->sandbox = *sandbox;
  data_->interface = s->traverse_get<sol::table>(names::node_root, names::node_interface);
  lua_sethook(s->lua_state(), &check_stop, LUA_MASKCOUNT | LUA_MASKCALL, stop_check_interval);
}

///
///
auto state_owner::lock() const -> state
{
  return state{data_};
}

///
///
auto state_owner::shutdown() const noexcept -> void
{
  data_->shutdown_flag = true;
}

} // namespace bibstd::lua
