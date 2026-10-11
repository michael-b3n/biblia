#pragma once

#include "bibstd/system/hotkey_common.hpp"
#include "bibstd/util/scope_guard.hpp"

#include <functional>

namespace bibstd::system
{

///
/// Generic hotkey class.
///
class hotkey final
{
public: // Typedefs
  using key = hotkey_common::key;
  using key_modifier = hotkey_common::key_modifier;

public: // Static modifiers
  ///
  /// Initialize hotkey registration. Multiple calls to this function are allowed and will be bound to existing
  /// shared scope guard. The returned guard will uninitialize the hotkey registration on destruction.
  /// \return guard to uninitialize hotkey registration on destruction
  ///
  static auto init() -> util::shared_scope_guard;

  ///
  /// Register global callback to specified key and key modifier. It is called on the thread
  /// of the implementation and has to return quickly, whatever takes time is queued by the caller.
  ///
  static auto register_callback(key key, key_modifier mod, std::function<void()>&& callback) -> void;

  ///
  /// Unregister global callback for specified key and key modifier.
  ///
  static auto unregister_callback(key key, key_modifier mod) -> void;
};

} // namespace bibstd::system
