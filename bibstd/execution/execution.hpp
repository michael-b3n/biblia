#pragma once

// beman.execution implements std::execution of C++26. Include this header instead of the
// library, the alias below is the one place to change once the standard library ships it.
#include <beman/execution/detail/run_loop.hpp>
#include <beman/execution/execution.hpp>
#include <beman/execution/stop_token.hpp>

namespace bibstd
{

namespace ex = beman::execution;

} // namespace bibstd
