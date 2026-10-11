#include "bibstd/execution/launch.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"

namespace bibstd::execution::detail
{

///
///
auto log_launch_error(const std::exception_ptr& error) noexcept -> void
{
  try
  {
    std::rethrow_exception(error);
  }
  catch(...)
  {
    LOG_ERROR("launched work failed: {}", util::exception_report());
  }
}

} // namespace bibstd::execution::detail
