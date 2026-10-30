#pragma once

#include <functional>
#include <memory>

namespace bibstd::util
{

///
/// Move-only guard calling a callable on destruction or reset.
///
class scope_guard final
{
  // Variables
  std::move_only_function<void()> on_destruction_;

public: // Typedefs
  using on_destruction_type = decltype(on_destruction_);

public: // Constructors
  scope_guard() = default;
  scope_guard(on_destruction_type&& on_destruction);
  ~scope_guard() noexcept;
  scope_guard(scope_guard&& rhs) noexcept;
  scope_guard(const scope_guard&) = delete;

public: // Operators
  auto operator=(scope_guard&& rhs) & noexcept -> scope_guard&;
  auto operator=(const scope_guard&) -> scope_guard& = delete;

public: // Modifiers
  ///
  /// Reset the guard, calling the callable if it exists.
  ///
  auto reset() -> void;

private: // Implementation
  auto destruct() -> void;
};

///
/// Shared scope guard class. This class allows multiple scope_guard instances to share the same underlying
/// callable, ensuring that the callable is only executed once when all guards are destroyed or reset.
/// This is useful for cases where multiple components need to share the same cleanup logic.
///
class shared_scope_guard final
{
  // Variables
  bool is_initial_instance_{false};
  scope_guard instance_guard_;

public: // Typedefs
  ///
  /// A helper class to create a bound scope exit action instance.
  /// Multiple calls to the create function will return a guard that is bound to the same underlying instance,
  /// ensuring that the on_destruction callback is only called once when all guards are destroyed or reset.
  ///
  class creator final
  {
    // Variables
    std::weak_ptr<scope_guard> instance_;

  public: // Constructors
    creator() = default;

  public: // Operations
    ///
    /// Create a bound scope exit action instance.
    /// \return A scope_guard instance bound to the same underlying
    /// instance as other guards created by this function
    ///
    auto create(scope_guard::on_destruction_type&& on_destruction) -> shared_scope_guard;
  };

public: // Constructors
  shared_scope_guard() = default;
  ~shared_scope_guard() noexcept = default;
  shared_scope_guard(shared_scope_guard&&) noexcept = default;
  shared_scope_guard(const shared_scope_guard&) = delete;

public: // Operators
  auto operator=(shared_scope_guard&&) & noexcept -> shared_scope_guard& = default;
  auto operator=(const shared_scope_guard&) -> shared_scope_guard& = delete;

public: // Accessors
  ///
  /// Check if the guard is the initial guard holding the callable.
  /// \return true if the guard is the initial guard, false otherwise
  ///
  [[nodiscard]] auto is_initial_instance() const -> bool;

public: // Modifiers
  ///
  /// Reset the guard, calling the callable if it exists.
  ///
  auto reset() -> void;

private: // Constructors
  shared_scope_guard(scope_guard instance_guard, bool is_initial_instance);
};

} // namespace bibstd::util
