#include "bibstd/framework/queued_connections.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"

namespace bibstd::framework
{

///
///
queued_connections::~queued_connections() noexcept
{
  try
  {
    disconnect();
  }
  catch(...)
  {
    LOG_ERROR("queued_connections destruction exception: {}", util::exception_report());
  }
}

///
///
auto queued_connections::disconnect() -> void
{
  connections_.clear();
  {
    const auto lock = std::scoped_lock{gate_->mtx};
    gate_->closed = true;
  }
  scope_.stop();
}

} // namespace bibstd::framework
