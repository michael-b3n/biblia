#include "bibstd/execution/scope.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"

#include <utility>

namespace bibstd::execution
{

///
///
scope::~scope() noexcept
{
  stop();
}

///
///
auto scope::token() noexcept -> token_type
{
  return scope_.get_token();
}

///
///
auto scope::stop() noexcept -> void
{
  const auto lock = std::scoped_lock{mtx_};
  if(std::exchange(stopped_, true))
  {
    return;
  }
  try
  {
    scope_.request_stop();
    ex::sync_wait(scope_.join());
  }
  catch(...)
  {
    LOG_ERROR("scope exception: {}", util::exception_report());
  }
}

} // namespace bibstd::execution
