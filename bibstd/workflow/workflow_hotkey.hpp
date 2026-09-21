#pragma once

#include "bibstd/signal/common.hpp"
#include "bibstd/system/hotkey_common.hpp"
#include "bibstd/util/scope_guard.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_settings.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bibstd::workflow
{

///
/// Workflow hotkey. This workflow manages the registration of a hotkey and the corresponding callback.
///
class workflow_hotkey final : public workflow_base<void>
{
  // Typedefs
  using hotkey_type = system::hotkey_common;

  template<typename T>
  using setting_type = workflow_settings::setting_non_owning_ptr_type<T>;

  ///
  /// Everything one callback path is made of: the signal the callback is called by, the settings
  /// naming the hotkey and the combination that hotkey is registered with at the moment.
  ///
  struct callback_data final
  {
    std::shared_ptr<signal::signal_type<void()>> shared_sig;
    setting_type<hotkey_type::key_modifier> modifier_setting;
    setting_type<hotkey_type::key> key_setting;
    std::optional<std::pair<hotkey_type::key, hotkey_type::key_modifier>> registered;
    std::vector<signal::scoped_connection_type> connections;
  };

  // Variables
  const std::shared_ptr<workflow_settings> workflow_settings_;
  const util::shared_scope_guard thread_pool_guard_;
  const util::shared_scope_guard hotkey_guard_;
  mutable std::mutex mtx_;
  std::unordered_map<std::string, callback_data> callbacks_;

public: // Typedefs
  using path_type = std::string;
  using key = hotkey_type::key;
  using key_modifier = hotkey_type::key_modifier;
  using shared_sig_type = decltype(callback_data::shared_sig);

public: // Structors
  workflow_hotkey(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_hotkey() noexcept override;

public: // Accessors
  ///
  /// Get the registered callback paths.
  /// \return registered callback paths
  ///
  [[nodiscard]] auto available_callbacks() const -> std::vector<std::string>;

public: // Modifiers
  ///
  /// Register a callback to a specific path and bind it to a hotkey. \p default_modifier and \p default_key
  /// name the hotkey the callback is bound to as long as the user did not choose another one.
  /// \return The shared signal associated with the path
  ///
  [[nodiscard]] auto register_callback(const path_type& path, key_modifier default_modifier, key default_key)
    -> shared_sig_type;

private: // Implementation
  auto apply_hotkey(const path_type& path) -> void;
};

} // namespace bibstd::workflow
