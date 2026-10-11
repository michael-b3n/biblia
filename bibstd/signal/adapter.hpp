#pragma once

#include "bibstd/signal/common.hpp"

#include <concepts>
#include <functional>

namespace bibstd::signal
{
namespace detail
{
// clang-format off
struct any_return_type{};
struct any_argument_type{};
using any_signal_type = signal_type<any_return_type(any_argument_type)>;
// clang-format on
} // namespace detail

///
/// Concept for what queues the slots of the adapter, \see queued_connections.
///
template<typename T>
concept connector_kind = requires(
  T t,
  detail::any_signal_type& sig,
  detail::any_signal_type::slot_type slot,
  detail::any_signal_type::extended_slot_type ext_slot
) {
  { t.connect(sig, std::move(slot)) } -> std::same_as<void>;
  { t.connect_extended(sig, std::move(ext_slot)) } -> std::same_as<void>;
};

///
/// Signal adapter class.
///
template<std::default_initializable T>
class adapter
{
  // Variables
  mutable T sigs_; // mutable to be able to fire and connect from constant members

public: // Typedefs
  using signals_type = T;

public: // Structors
  adapter() = default;
  virtual ~adapter() noexcept = default;

public: // Modifiers
  ///
  /// Connect a slot to the signal specified by the projection.
  /// \return scoped connection corresponding to connected slot
  ///
  [[nodiscard]] auto connect(auto sig_projection, auto&& slot) const -> scoped_connection_type;

  ///
  /// Connect a slot to the signal specified by the projection, allowing extended functionality.
  /// \return connection corresponding to connected slot
  ///
  auto connect_extended(auto sig_projection, auto&& slot) const -> connection_type;

  ///
  /// Connect a slot to the signal specified by the projection, the slot is queued by \p connections
  /// instead of being called by the emitting thread. The connection ends with \p connections.
  ///
  auto connect_queued(auto sig_projection, auto&& slot, connector_kind auto& connections) const -> void;

  ///
  /// Connect an extended slot to the signal specified by the projection, the slot is queued by \p connections
  /// instead of being called by the emitting thread. The connection ends with \p connections.
  ///
  auto connect_queued_extended(auto sig_projection, auto&& slot, connector_kind auto& connections) const -> void;

protected: // Accessors
  ///
  /// Emit the signal specified by the projection, passing the provided arguments to the connected slots.
  /// \return result of the signal call, if any
  ///
  auto notify(auto sig_projection, auto&&... args) -> auto;
};

///
///
template<std::default_initializable T>
auto adapter<T>::connect(auto sig_projection, auto&& slot) const -> scoped_connection_type
{
  return std::invoke(sig_projection, sigs_).connect(std::forward<decltype(slot)>(slot));
}

///
///
template<std::default_initializable T>
auto adapter<T>::connect_extended(auto sig_projection, auto&& slot) const -> connection_type
{
  return std::invoke(sig_projection, sigs_).connect_extended(std::forward<decltype(slot)>(slot));
}

///
///
template<std::default_initializable T>
auto adapter<T>::connect_queued(auto sig_projection, auto&& slot, connector_kind auto& connections) const -> void
{
  connections.connect(std::invoke(sig_projection, sigs_), std::forward<decltype(slot)>(slot));
}

///
///
template<std::default_initializable T>
auto adapter<T>::connect_queued_extended(auto sig_projection, auto&& slot, connector_kind auto& connections) const -> void
{
  connections.connect_extended(std::invoke(sig_projection, sigs_), std::forward<decltype(slot)>(slot));
}

///
///
template<std::default_initializable T>
auto adapter<T>::notify(auto sig_projection, auto&&... args) -> auto
{
  return std::invoke(sig_projection, sigs_)(std::forward<decltype(args)>(args)...);
}

} // namespace bibstd::signal
