#pragma once

#include "bibstd/execution/execution.hpp"
#include "bibstd/execution/scope.hpp"

#include <concepts>
#include <exception>
#include <type_traits>
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
/// Run \p work on the thread pool, detached in \p scope which waits for it. An exception it throws is logged.
///
template<std::invocable F>
  requires std::is_void_v<std::invoke_result_t<F>>
auto launch(F&& work, scope& scope) -> void
{
  ex::spawn(
    ex::schedule(ex::get_parallel_scheduler()) | ex::then(std::forward<F>(work)) | ex::upon_error(&detail::log_launch_error),
    scope.token()
  );
}

} // namespace bibstd::execution
