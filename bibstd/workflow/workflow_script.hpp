#pragma once

#include "bibstd/lua/state.hpp"
#include "bibstd/lua/state_owner.hpp"
#include "bibstd/lua/value_cast.hpp"
#include "bibstd/signal/adapter.hpp"
#include "bibstd/signal/common.hpp"
#include "bibstd/util/path.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_settings.hpp"
#include "bibstd/workflow/workflow_settings_base.hpp"

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace bibstd::workflow
{

///
/// Manifest of a function a script offers, defined by the workflow calling it: the static id naming the function,
/// e.g. "scripture.passage", its input and its output, usually a lua::script_table.
///
template<typename M>
concept script_manifest = requires {
  { M::id } -> std::convertible_to<util::path>;
} && lua::value_castable<typename M::input> && lua::value_castable<typename M::output>;

///
/// Settings corresponding to workflow script.
///
class workflow_script_settings final : public workflow_settings_base
{
public: // Structors
  workflow_script_settings(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_script_settings() noexcept override = default;

public: // Constants
  static constexpr auto folder_name = "scripts";

public: // Variables
  const setting_type<bool> enabled;
  const std::filesystem::path folder;
};

///
/// Signals emitted by workflow script.
///
struct workflow_script_sigs final
{
  signal::signal_type<void()> scripts_loaded;
};

///
/// Workflow script, owns the Lua state and loads the bundled and user scripts \see load_scripts, doc/lua_scripts.md.
/// Workflows register below root.interface.workflow last in their constructor and keep the registration as their
/// last member, \see lua::registration.
/// Signal IDs to connect to:
/// - scripts_loaded: Emitted under the Lua lock once the scripts are loaded, also if none are.
///
class workflow_script final
  : public workflow_base<workflow_script_settings>
  , public signal::adapter<workflow_script_sigs>
{
  // Typedefs
  struct script_info final
  {
    std::string description;
    std::set<util::path> functions;
  };

  // Constants
  static constexpr auto lua_node = "workflow";    // below root, the node of all workflow registrations
  static constexpr auto setting_node = "setting"; // below lua_node, the node of all settings

  // Variables
  const lua::state_owner state_owner_;
  mutable std::mutex mtx_;
  std::map<std::string, script_info> scripts_; // the loaded scripts by name
  std::jthread loader_;                        // last, it joins before the members it uses are destroyed

public: // Typedefs
  using script_name_type = decltype(scripts_)::key_type;
  using scripts_type = decltype(scripts_);

public: // Structors
  workflow_script(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_script() noexcept override;

public: // Accessors
  ///
  /// \return the locked Lua state
  ///
  [[nodiscard]] auto state() const -> lua::state;

  ///
  /// \return the loaded scripts offering a function for each manifest \p M, all of them without \p M, by name
  ///
  template<script_manifest... M>
  [[nodiscard]] auto scripts() const -> scripts_type;

public: // Modifiers
  ///
  /// Load the bundled scripts and, if enabled, the user scripts outside of the calling thread, and emit
  /// scripts_loaded. Called once the workflows registered, and again to load the scripts anew: a loading still
  /// running ends after its script first.
  ///
  auto load_scripts() -> void;

  ///
  /// Copy the Lua script \p files into the script folder, replacing a file of the same name. They run once the
  /// scripts are loaded anew, \see load_scripts.
  /// \return number of files copied, the others are logged
  ///
  [[nodiscard]] auto import_scripts(const std::vector<std::filesystem::path>& files) -> std::size_t;

  ///
  /// Register a function for scripts at workflow.\p p, e.g. "web.fetch", \see lua::state::register_function
  /// \return the registration to keep, one of nothing if \p p can not be registered, the reason is logged
  ///
  template<lua::function_castable F>
  [[nodiscard]] auto register_function(const util::path& p, F&& function) const -> lua::registration;

  ///
  /// Register \p setting for scripts at workflow.setting and its own path, \see lua::state::register_setting
  /// \return the registration to keep, one of nothing if the path can not be registered, the reason is logged
  ///
  template<framework::underlying_setting_type T>
  [[nodiscard]] auto register_setting(framework::setting<T>& setting) const -> lua::registration;

  ///
  /// Run the function of the manifest \p M of the script named \p script. Blocks until it returns, it may wait
  /// e.g. for a web page.
  /// \return its output, or std::nullopt if it failed, is missing or its output does not fit \p M
  ///
  template<script_manifest M>
  [[nodiscard]] auto run(const script_name_type& script, const typename M::input& input) const
    -> std::optional<typename M::output>;

  ///
  /// Stop the scripts with force, no script runs from now on. Called before the app ends, so no workflow waits
  /// for a script that never returns once it is destroyed, \see lua::state_owner::shutdown
  ///
  auto shutdown() const noexcept -> void;

private: // Implementation
  [[nodiscard]] static auto lua_path(const util::path& p) -> util::path;
  [[nodiscard]] auto scripts(const std::vector<util::path>& ids) const -> scripts_type;
  auto run(
    const script_name_type& script,
    const util::path& id,
    const std::function<sol::object(sol::state_view)>& input,
    const std::function<bool(const sol::object&)>& output
  ) const -> void;
  auto load(const std::stop_token& stop_token) -> void;
  auto load_folder(const std::filesystem::path& folder, const std::stop_token& stop_token) -> void;
  auto load_script(std::string_view file, std::string_view code) -> void;
  auto add_script(const lua::state& state, const std::string& file, const sol::object& returned) -> void;
};

///
///
template<script_manifest... M>
auto workflow_script::scripts() const -> scripts_type
{
  return scripts(std::vector<util::path>{util::path{M::id}...});
}

///
///
template<lua::function_castable F>
auto workflow_script::register_function(const util::path& p, F&& function) const -> lua::registration
{
  return state().register_function(lua_path(p), std::forward<F>(function));
}

///
///
template<framework::underlying_setting_type T>
auto workflow_script::register_setting(framework::setting<T>& setting) const -> lua::registration
{
  return state().register_setting(lua_path(util::path{setting_node} / util::path{setting.path}), setting);
}

///
///
template<script_manifest M>
auto workflow_script::run(const script_name_type& script, const typename M::input& input) const
  -> std::optional<typename M::output>
{
  auto output = std::optional<typename M::output>{};
  run(
    script,
    util::path{M::id},
    [&](sol::state_view lua) { return lua::value_cast<typename M::input>::to(lua, input); },
    [&](const sol::object& object)
    {
      output = lua::value_cast<typename M::output>::from(object);
      return output.has_value();
    }
  );
  return output;
}

} // namespace bibstd::workflow
