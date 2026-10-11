#pragma once

#include "bibstd/execution/launch.hpp"
#include "bibstd/execution/scope.hpp"
#include "bibstd/signal/common.hpp"
#include "bibstd/signal/connection_store.hpp"

#include <atomic>
#include <concepts>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

namespace bibstd::framework
{

///
/// Connections to signals whose slots run on the thread pool instead of on the emitting thread,
/// possibly several at the same time. The connections end with this object or on disconnect.
/// No slot connected through it runs once disconnect returned, a slot queued by then is dropped.
/// \see signal::adapter::connect_queued and signal::adapter::connect_queued_extended
///
class queued_connections final
{
  // Typedefs
  ///
  /// Shared with the connected slots, an emission may outlive this object.
  ///
  struct gate final
  {
    std::mutex mtx;
    std::atomic_bool closed{false};
  };

  // Variables
  const std::shared_ptr<gate> gate_{std::make_shared<gate>()};
  signal::connection_store connections_;
  execution::scope scope_;

public: // Structors
  queued_connections() = default;
  ~queued_connections() noexcept;

public: // Modifiers
  ///
  /// Connect a slot to a signal. An emission copies its arguments and queues the slot.
  /// \see adapter::connect_queued
  ///
  template<signal::signal_like T>
  auto connect(T& sig, typename T::slot_type slot) -> void;

  ///
  /// Connect an extended slot to a signal. An emission copies its arguments and queues the slot.
  /// \see adapter::connect_queued_extended
  ///
  template<signal::signal_like T>
  auto connect_extended(T& sig, typename T::extended_slot_type slot) -> void;

  ///
  /// Disconnect all slots and wait for the ones that are running. Nothing can be connected afterwards.
  /// \warning Must not be called by a slot connected through this object.
  ///
  auto disconnect() -> void;

private: // Implementation
  ///
  /// \return \p slot as function of the signal, which queues the slot
  ///
  template<typename F, typename S>
  [[nodiscard]] auto queued(S slot) -> F;
};

///
///
template<signal::signal_like T>
auto queued_connections::connect(T& sig, typename T::slot_type slot) -> void
{
  // Nobody is left to take the value a queued slot returns.
  static_assert(std::is_void_v<typename signal::signal_signature<T>::return_type>);
  using function_type = signal::to_functional<T, std::function>::type;
  static_assert(std::is_convertible_v<decltype(slot), function_type>);
  static_assert(std::copyable<decltype(slot)>);

  connections_.store(sig.connect(queued<function_type>(std::move(slot))));
}

///
///
template<signal::signal_like T>
auto queued_connections::connect_extended(T& sig, typename T::extended_slot_type slot) -> void
{
  // Nobody is left to take the value a queued slot returns.
  static_assert(std::is_void_v<typename signal::signal_signature<T>::return_type>);
  using function_type = signal::to_functional<T, std::function>::type_extended;
  static_assert(std::is_convertible_v<decltype(slot), function_type>);
  static_assert(std::copyable<decltype(slot)>);

  connections_.store(sig.connect_extended(queued<function_type>(std::move(slot))));
}

///
///
template<typename F, typename S>
auto queued_connections::queued(S slot) -> F
{
  return [this, gate = gate_, slot = std::move(slot)](auto&&... args)
  {
    // std::make_tuple copies the arguments, the slot runs after the emission returned.
    auto task = [gate, slot, args = std::make_tuple(std::forward<decltype(args)>(args)...)]() mutable
    {
      if(!gate->closed)
      {
        std::apply(std::move(slot), std::move(args));
      }
    };
    // disconnect closes the gate before the scope stops, so `this` is alive while the gate is open.
    const auto lock = std::scoped_lock{gate->mtx};
    if(!gate->closed)
    {
      execution::launch(std::move(task), scope_);
    }
  };
}

} // namespace bibstd::framework
