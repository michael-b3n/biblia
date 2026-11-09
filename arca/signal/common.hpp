#pragma once

#include <boost/signals2.hpp>

#include <functional>

namespace bibstd::signal
{

///
/// Type alias for signal type from boost::signals2.
/// A signal can be used to connect slots (callback functions) to it.
/// Calling a signal will call all connected slots.
///
template<typename T>
using signal_type = boost::signals2::signal<T>;

///
/// Type trait to check if a type is a signal type.
///
template<typename T>
struct is_signal_type final : std::false_type
{};
template<typename T>
struct is_signal_type<signal_type<T>> final : std::true_type
{};

///
/// Boolean flag to check if a type is a signal type.
/// \see is_signal_type
///
template<typename T>
inline constexpr auto is_signal_type_v = is_signal_type<std::remove_cvref_t<T>>::value;

///
/// Signal concept to check if a type is a signal type.
///
template<typename T>
concept signal_like = is_signal_type_v<T>;

namespace detail
{

///
/// Helper for `signal_signature` to extract signal signature from a signal type.
///
template<typename T>
struct signal_signature_extractor;
template<typename R, typename... Args>
struct signal_signature_extractor<R(Args...)> final
{
  using return_type = R;
  using args_tuple = std::tuple<Args...>;
};

} // namespace detail

///
/// Type trait to access signal signature.
/// The signal signature consists of the return type and the argument types of the signal.
///
template<signal_like T>
struct signal_signature
{
private:
  using signature = detail::signal_signature_extractor<typename T::slot_type::signature_type>;
  using signature_extended = detail::signal_signature_extractor<typename T::extended_slot_type::signature_type>;
  static_assert(std::is_same_v<typename signature::return_type, typename signature_extended::return_type>);

public:
  using return_type = typename signature::return_type;
  using args_tuple = typename signature::args_tuple;
  using extended_args_tuple = typename signature_extended::args_tuple;
};

///
/// Type trait to convert a signal type to a functional type (std::function or move_only_function).
/// Provides type aliases for the corresponding functional types for both the regular and extended slot types of the signal.
/// \tparam T signal type to convert to functional type
/// \tparam F functional type to convert to (std::function or move_only_function)
///
template<signal_like T, template<typename...> typename F>
struct to_functional;
template<typename T>
struct to_functional<T, std::function> final
{
  using type = std::function<typename T::slot_type::signature_type>;
  using type_extended = std::function<typename T::extended_slot_type::signature_type>;
};
template<typename T>
struct to_functional<T, std::move_only_function> final
{
  using type = std::move_only_function<typename T::slot_type::signature_type>;
  using type_extended = std::move_only_function<typename T::extended_slot_type::signature_type>;
};

///
/// Type alias for connection type from boost::signals2.
/// A connection can be used to disconnect a slot from a signal.
///
using connection_type = boost::signals2::connection;

///
/// Boost::signals2 scoped connection type.
/// Disconnects the connection when it goes out of scope.
///
using scoped_connection_type = boost::signals2::scoped_connection;

///
/// Boost::signals2 shared connection blocker.
/// This is used to block a signal temporarily.
///
using connection_block_type = boost::signals2::shared_connection_block;

} // namespace bibstd::signal
