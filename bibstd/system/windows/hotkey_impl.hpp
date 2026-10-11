#pragma once

#include "bibstd/system/hotkey_common.hpp"
#include "bibstd/system/windows/message_loop.hpp"
#include "bibstd/util/scope_guard.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <utility>

namespace bibstd::system
{

///
/// Hotkey implementation class for the windows OS.
///
class hotkey_impl final
{
  // Variables
  inline static std::unique_ptr<message_loop> loop_{};
  // Both maps belong to the loop thread, the hotkeys are registered with its window
  inline static std::map<int, std::function<void()>> callback_map_{};
  inline static std::map<std::pair<hotkey_common::key, hotkey_common::key_modifier>, int> id_map_{};

public: // Static modifiers
  ///
  /// \see hotkey::init
  ///
  static auto init() -> util::shared_scope_guard;

  ///
  /// \see hotkey::register_callback
  ///
  static auto register_callback(hotkey_common::key key, hotkey_common::key_modifier mod, std::function<void()>&& callback)
    -> void;

  ///
  /// \see hotkey::unregister_callback
  ///
  static auto unregister_callback(hotkey_common::key key, hotkey_common::key_modifier mod) -> void;

private: // Static helpers
  ///
  /// Handle a message of the loop window, a pressed hotkey calls its callback.
  ///
  static auto handle_message(unsigned int message, std::uintptr_t wparam) -> void;
};

} // namespace bibstd::system
