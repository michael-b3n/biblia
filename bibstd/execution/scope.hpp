#pragma once

#include "bibstd/execution/execution.hpp"

#include <mutex>

namespace bibstd::execution
{

///
/// Scope holding launched work, \see launch. Destruction requests a stop and
/// waits for the work, so the work may refer to whatever outlives the scope.
///
class scope final
{
  // Variables
  mutable std::mutex mtx_;
  bool stopped_{false};
  ex::counting_scope scope_;

public: // Typedefs
  using token_type = ex::counting_scope::token;

public: // Structors
  scope() = default;
  ~scope() noexcept;

public: // Accessors
  ///
  /// \return token work is launched with
  ///
  [[nodiscard]] auto token() noexcept -> token_type;

public: // Modifiers
  ///
  /// Request a stop and wait for the launched work. Work launched afterwards is dropped.
  /// \warning Must not be called by the work itself.
  ///
  auto stop() noexcept -> void;
};

} // namespace bibstd::execution
