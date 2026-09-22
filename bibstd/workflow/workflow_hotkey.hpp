#pragma once

#include "bibstd/signal/common.hpp"
#include "bibstd/signal/synchronized_executor.hpp"
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
/// Workflow hotkey. This workflow manages the registration of a hotkey.
/// Callbacks shall be registered using a path and connecting to the
/// returned signal. The hotkey can be changed by the user via workflow settings.
///
class workflow_hotkey final : public workflow_base<void>
{
  // Typedefs
  using hotkey_type = system::hotkey_common;
  template<typename T>
  using setting_type = workflow_settings::setting_non_owning_ptr_type<T>;

  struct callback_data final
  {
    std::shared_ptr<signal::signal_type<void()>> shared_sig;

    // Both settings `modifier_setting` and `key_setting` are created on registration of a callback.
    // Changing the settings trigger an async update of the hotkey registration. There is no guarantee
    // that the hotkey is registered with the new combination immediately after the setting is changed.
    // Changing these settings via object client (using workflow_settings) is therefore not recommended.
    setting_type<hotkey_type::key_modifier> modifier_setting;
    setting_type<hotkey_type::key> key_setting;

    std::optional<std::pair<hotkey_type::key, hotkey_type::key_modifier>> registered;
    signal::synchronized_executor executor;
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

public: // Modifiers
  ///
  /// Register a callback to a specific path and bind it to a hotkey. \p default_modifier and \p default_key
  /// name the hotkey the callback is bound to as long as the user did not choose another one. For the
  /// modifier and the key settings are created and exposed to the user. The settings are not exposed by `this`.
  /// \return The shared signal associated with the path
  ///
  [[nodiscard]] auto register_callback(const path_type& path, key_modifier default_modifier, key default_key)
    -> shared_sig_type;

private: // Implementation
  auto apply_hotkey_impl(const path_type& path) -> void;
};

} // namespace bibstd::workflow
