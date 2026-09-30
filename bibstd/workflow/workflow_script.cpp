#include "bibstd/workflow/workflow_script.hpp"
#include "bibstd/lua/names.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bibstd::workflow
{
namespace
{

///
/// \return the Lua scripts directly inside \p folder, sorted by name
///
[[nodiscard]] auto lua_files(const std::filesystem::path& folder) -> std::vector<std::filesystem::path>
{
  auto error = std::error_code{};
  const auto is_lua_file = [&](const auto& entry)
  { return entry.is_regular_file(error) && entry.path().extension() == ".lua"; };
  auto files = std::filesystem::directory_iterator{folder, error} | std::views::filter(is_lua_file) |
               std::views::transform([](const auto& entry) { return entry.path(); }) | std::ranges::to<std::vector>();
  std::ranges::sort(files);
  return files;
}

///
/// \return the content of \p file
///
[[nodiscard]] auto read(const std::filesystem::path& file) -> std::optional<std::string>
{
  auto stream = std::ifstream{file, std::ios::binary};
  if(!stream)
  {
    LOG_WARN("failed to open file: path=\"{}\"", file.string());
    return std::nullopt;
  }
  return std::string{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

} // namespace

///
///
workflow_script_settings::workflow_script_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : framework::settings_base{std::move(workflow_settings)}
  , enabled{workflow_settings_->create_setting("script.enabled", true)}
  , folder{workflow_settings_->create_setting("script.folder", workflow_settings_->data_folder() / default_folder_name)}
{
}

///
///
workflow_script::workflow_script(std::shared_ptr<workflow_settings> workflow_settings)
  : workflow_base{std::move(workflow_settings)}
{
}

///
///
workflow_script::~workflow_script() noexcept
{
  // First, the loading ends after the script it runs, which is stopped as a script may run endlessly.
  // The loader joins once destroyed.
  loader_.request_stop();
  shutdown();
  // Waits for the running script
  [[maybe_unused]] const auto state = this->state();
}

///
///
auto workflow_script::state() const -> lua::state
{
  return state_owner_.lock();
}

///
///
auto workflow_script::lua_path(const util::path& p) -> util::path
{
  return util::path{std::format("{}.{}", lua_node, p.string())};
}

///
///
auto workflow_script::scripts(const std::vector<util::path>& ids) const -> scripts_type
{
  const auto offers_all = [&ids](const auto& entry)
  { return std::ranges::all_of(ids, [&](const auto& id) { return entry.second.functions.contains(id); }); };
  const auto lock = std::scoped_lock{mtx_};
  return scripts_ | std::views::filter(offers_all) | std::ranges::to<scripts_type>();
}

///
///
auto workflow_script::run(
  const util::identifier& script,
  const util::path& id,
  const std::function<sol::object(sol::state_view)>& input,
  const std::function<bool(const sol::object&)>& output
) const -> void
{
  auto state = state_owner_.lock();
  const auto function = state->traverse_get<sol::optional<sol::protected_function>>(
    lua::names::node_root, lua::names::node_system, lua::names::node_scripts, script.string(), id.string()
  );
  if(!function)
  {
    return;
  }
  if(const auto result = state.call(*function, input(*state)); result && !output(*result))
  {
    LOG_ERROR("lua script output rejected: script=\"{}\", function=\"{}\"", script.string(), id.string());
  }
}

///
///
auto workflow_script::shutdown() const noexcept -> void
{
  state_owner_.shutdown();
}

///
///
auto workflow_script::load_scripts() -> void
{
  loader_ = std::jthread{[this](const std::stop_token& stop_token) { load(stop_token); }};
}

///
///
auto workflow_script::load(const std::stop_token& stop_token) -> void
{
  // Loaded anew, so a script gone from the folder is gone
  {
    auto state = state_owner_.lock();
    state->traverse_get<sol::table>(lua::names::node_root, lua::names::node_system)[lua::names::node_scripts] = sol::lua_nil;
    const auto lock = std::scoped_lock{mtx_};
    scripts_.clear();
  }
  // Script failures are logged by the state, this only catches faults of the app, e.g. a vanishing folder.
  // Either way the waiting workflows learn that loading is over.
  try
  {
    if(settings().enabled->value())
    {
      const auto folder = settings().folder->value();
      auto error = std::error_code{};
      if(!std::filesystem::exists(folder, error))
      {
        if(!std::filesystem::create_directories(folder, error))
        {
          LOG_ERROR("failed to create script folder: folder=\"{}\", error={}", folder.generic_string(), error.message());
        }
      }
      load_folder(folder, stop_token);
    }
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
  }
  const auto state = state_owner_.lock();
  notify(&signals_type::scripts_loaded);
}

///
///
auto workflow_script::load_folder(const std::filesystem::path& folder, const std::stop_token& stop_token) -> void
{
  std::ranges::for_each(
    lua_files(folder) | std::views::take_while([&](const auto&) { return !stop_token.stop_requested(); }),
    [&](const auto& file)
    {
      // Per script, so a workflow registering meanwhile waits for one script only
      auto state = state_owner_.lock();
      const auto code = read(file);
      const auto name = file.filename().string();
      if(const auto description = code ? state.run_script(name, *code) : std::nullopt)
      {
        add_script(state, name, *description);
      }
    }
  );
}

///
///
auto workflow_script::add_script(const lua::state& state, const std::string& file, const sol::object& description) -> void
{
  if(description.get_type() != sol::type::table)
  {
    LOG_INFO("lua script offers nothing: file=\"{}\"", file);
    return;
  }
  auto table = description.as<sol::table>();
  const auto text = [&](const std::string_view key) { return lua::value_cast<std::string>::from(table.get<sol::object>(key)); };
  const auto id = text(lua::names::script_id).and_then([](const auto& t) { return util::identifier::from(t); });
  const auto name = text(lua::names::script_name);
  const auto functions = table.get<sol::object>(lua::names::script_functions);
  if(!id || !name || name->empty() || functions.get_type() != sol::type::table)
  {
    LOG_ERROR("lua script rejected: file=\"{}\", it returns no name or no table of functions", file);
    return;
  }
  // The first one keeps the id, a script is known by nothing else
  if(const auto lock = std::scoped_lock{mtx_}; scripts_.contains(*id))
  {
    LOG_ERROR("lua script rejected: file=\"{}\", id=\"{}\" is taken", file, id->string());
    return;
  }
  // Only named functions, so a workflow finds nothing it can not run
  auto kept = state->create_table();
  auto function_names = std::set<util::path>{};
  functions.as<sol::table>().for_each(
    [&](const sol::object& key, const sol::object& value)
    {
      const auto function = key.get_type() == sol::type::string ? util::path{key.as<std::string>()} : util::path{};
      if(function.empty() || value.get_type() != sol::type::function)
      {
        LOG_ERROR("lua script entry rejected: script=\"{}\", no named function", id->string());
        return;
      }
      kept[function.string()] = value;
      function_names.insert(function);
    }
  );
  sol::table system = state->traverse_get<sol::table>(lua::names::node_root, lua::names::node_system);
  system[lua::names::node_scripts].get_or_create<sol::table>()[id->string()] = kept;
  LOG_INFO(
    "lua script loaded: id=\"{}\", name=\"{}\", functions={}",
    id->string(),
    *name,
    function_names | std::views::transform(&util::path::string)
  );
  const auto lock = std::scoped_lock{mtx_};
  scripts_.insert_or_assign(*id, script_info{.name = *name, .functions = std::move(function_names)});
}

} // namespace bibstd::workflow
