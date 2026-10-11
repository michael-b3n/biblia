#pragma once

#include "bibstd/execution/execution.hpp"
#include "bibstd/execution/scope.hpp"

#include <exception>
#include <utility>

namespace bibstd::execution
{
namespace detail
{

///
/// Log the error launched work ended with, nobody else is left to receive it.
///
auto log_launch_error(const std::exception_ptr& error) noexcept -> void;

} // namespace detail

///
/// Start \p sender detached in \p scope, which waits for it and passes its stop request on.
///
template<ex::sender S>
auto launch(S&& sender, scope& scope) -> void
{
  ex::spawn(std::forward<S>(sender) | ex::upon_error(&detail::log_launch_error), scope.token());
}

} // namespace bibstd::execution
