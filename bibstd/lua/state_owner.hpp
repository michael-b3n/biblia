#pragma once

#include "bibstd/lua/state.hpp"

#include <memory>

namespace bibstd::lua
{

///
/// Owner of a Lua state, shared by all of its copies. Each copy can be used on any thread, \see lock.
/// The setup opens the libraries, creates the root table and runs the embedded init.lua.
///
class state_owner final
{
  // Variables
  std::shared_ptr<detail::state_data> data_;

public: // Structors
  state_owner();
  state_owner(const state_owner&) = default;
  auto operator=(const state_owner&) -> state_owner& = default;
  ~state_owner() noexcept = default;

public: // Modifiers
  ///
  /// Locks the state recursively, a C++ function called from a script may lock it again.
  /// \return the locked state
  ///
  [[nodiscard]] auto lock() const -> state;

  ///
  /// Shut the scripts down for good, from any thread and without the lock: root.system.shutdown_flag is set, the running
  /// code fails with an error at its next call and code started later fails right away.
  /// Catching the error does not keep a script running.
  ///
  auto shutdown() const noexcept -> void;
};

} // namespace bibstd::lua
