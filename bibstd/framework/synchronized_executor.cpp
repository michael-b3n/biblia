#include "bibstd/framework/synchronized_executor.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include <algorithm>

namespace bibstd::framework
{

///
///
synchronized_executor::synchronized_executor()
  : thread_pool_guard_{thread_pool::init()}
{
}

///
///
synchronized_executor::synchronized_executor(const thread_pool::strand_id_type strand_id)
  : thread_pool_guard_{thread_pool::init()}
  , strand_id_{strand_id}
{
}

///
///
synchronized_executor::~synchronized_executor() noexcept
{
  try
  {
    disconnect();
  }
  catch(...)
  {
    LOG_ERROR("synchronized_executor destruction exception: {}", util::exception_report());
  }
}

///
///
auto synchronized_executor::disconnect() -> void
{
  // Held until the end, so a second call from another thread waits for this one
  const auto lock = std::scoped_lock{mtx_};
  connections_.clear();
  auto syncs = std::move(syncs_);
  syncs_ = {};
  std::ranges::for_each(
    syncs,
    [](const auto& sync)
    {
      const auto lock = std::scoped_lock{sync->mtx};
      sync->disconnected = true;
    }
  );
}

///
///
auto synchronized_executor::exec(thread_pool::task_type&& task) const -> void
{
  if(strand_id_.has_value())
  {
    thread_pool::queue_task(std::move(task), *strand_id_);
  }
  else
  {
    thread_pool::queue_task(std::move(task));
  }
}

} // namespace bibstd::framework
