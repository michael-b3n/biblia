#pragma once

#include "bibstd/framework/thread_pool.hpp"
#include "bibstd/signal/common.hpp"
#include "bibstd/signal/connection_store.hpp"
#include "bibstd/util/scope_guard.hpp"

#include <concepts>
#include <memory>
#include <mutex>

namespace bibstd::framework
{

///
/// synchronized_executor used for executing signals in the default thread pool.
/// This executor contains a connection store that stores connections to signals
/// and disconnects them when destroyed. This store is thread-safe and guarantees
/// that any slots connected through this store will not be called after
/// disconnect returns, even if they are just about to be called.
/// \see signal::adapter::connect_queued and signal::adapter::connect_queued_extended
///
class synchronized_executor final
{
  // Typedefs
  struct sync_wrapper final
  {
    bool disconnected{false};
    std::mutex mtx;
  };

  // Variables
  const util::shared_scope_guard thread_pool_guard_;
  const std::optional<thread_pool::strand_id_type> strand_id_;
  mutable std::mutex mtx_;
  std::vector<std::shared_ptr<sync_wrapper>> syncs_;
  signal::connection_store connections_;

public: // Structors
  synchronized_executor();
  synchronized_executor(thread_pool::strand_id_type strand_id);
  ~synchronized_executor() noexcept;

public: // Modifiers
  ///
  /// Connects a slot to a signal and stores the connection in the store.
  /// The slot will be automatically disconnected when the synchronized_executor is destroyed or disconnect is called.
  /// \see adapter::connect_queued and adapter::connect_queued_extended
  ///
  template<signal::signal_like T>
  auto connect(T& sig, typename T::slot_type slot) -> void;

  ///
  /// Connects an extended slot to a signal and stores the connection in the store.
  /// The extended slot will be automatically disconnected when the synchronized_executor is destroyed or disconnect is called.
  /// \see adapter::connect_queued and adapter::connect_queued_extended
  ///
  template<signal::signal_like T>
  auto connect_extended(T& sig, typename T::extended_slot_type slot) -> void;

  ///
  /// Disconnects all connections in the store. Any slots connected through this store
  /// will not be called after disconnect returns, even if they are still connected to
  /// their signals, being called or just about to be called.
  ///
  /// \warning The program is ill-formed if any of the slots connected through this
  /// store are calling disconnect.
  ///
  auto disconnect() -> void;

private: // Implementation
  ///
  /// Called under the lock of the store.
  /// \return \p slot as function of the signal, run on the executor and never once disconnected
  ///
  template<signal::signal_like T, typename S>
  [[nodiscard]] auto queued(S slot) -> signal::to_functional<T, std::function>::type;

  ///
  /// Execute task in thread pool. If strand_id was specified in constructor,
  /// the task will be executed in the corresponding strand.
  ///
  auto exec(thread_pool::task_type&& task) const -> void;
};

///
///
template<signal::signal_like T>
auto synchronized_executor::connect(T& sig, typename T::slot_type slot) -> void
{
  const auto lock = std::scoped_lock{mtx_};
  connections_.store(sig.connect(queued<T>(std::move(slot))));
}

///
///
template<signal::signal_like T>
auto synchronized_executor::connect_extended(T& sig, typename T::extended_slot_type slot) -> void
{
  const auto lock = std::scoped_lock{mtx_};
  connections_.store(sig.connect_extended(queued<T>(std::move(slot))));
}

///
///
template<signal::signal_like T, typename S>
auto synchronized_executor::queued(S slot) -> signal::to_functional<T, std::function>::type
{
  // What a queued slot returns reaches nobody
  static_assert(std::is_void_v<typename signal::signal_signature<T>::return_type>);
  static_assert(std::is_convertible_v<S, typename signal::to_functional<T, std::function>::type>);
  static_assert(std::copyable<S>);

  auto se = std::make_shared<sync_wrapper>();
  syncs_.push_back(se);
  auto st = std::make_shared<sync_wrapper>();
  syncs_.push_back(st);
  return [this, sync_e = std::move(se), sync_t = std::move(st), slot = std::move(slot)](auto&&... args) mutable
  {
    // Copied, std::make_tuple decays the arguments
    auto args_tuple = std::make_tuple(std::forward<decltype(args)>(args)...);
    auto f = [sync_t, slot, args = std::move(args_tuple)]() mutable
    {
      // Held while the slot runs, so disconnect waits for it
      const auto lock = std::scoped_lock{sync_t->mtx};
      if(sync_t->disconnected)
      {
        return;
      }
      std::apply(std::move(slot), std::move(args));
    };
    // Queued, so no thread locks this mutex twice, and disconnect waits for the queueing. A task still queued
    // once disconnected does not run its slot, the task checks the flag under its own lock.
    const auto lock = std::scoped_lock{sync_e->mtx};
    if(sync_e->disconnected)
    {
      return;
    }
    exec(std::move(f));
  };
}

} // namespace bibstd::framework
