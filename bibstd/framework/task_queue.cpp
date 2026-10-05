#include "bibstd/framework/task_queue.hpp"

namespace bibstd::framework
{

///
///
task_queue::~task_queue() noexcept
{
  shutdown();
  const auto lock = std::scoped_lock{queue_mtx_, task_mtx_}; // wait until lock is freed
}

///
///
auto task_queue::empty() const -> bool
{
  const auto lock = std::scoped_lock{queue_mtx_};
  return task_queue_.empty();
}

///
///
auto task_queue::size() const -> std::size_t
{
  const auto lock = std::scoped_lock{queue_mtx_};
  return task_queue_.size();
}

///
///
auto task_queue::queue(task_type&& task) -> void
{
  {
    const auto lock = std::scoped_lock{queue_mtx_};
    if(shutdown_)
    {
      return;
    }
    task_queue_.emplace(std::forward<decltype(task)>(task));
  }
  task_cv_.notify_one();
}

///
///
auto task_queue::shutdown() -> void
{
  {
    const auto queue_lock = std::scoped_lock{queue_mtx_};
    shutdown_ = true;
    std::queue<task_type> empty_queue;
    std::swap(task_queue_, empty_queue);
  }
  task_cv_.notify_all();
}

///
///
auto task_queue::try_do_task() -> void
{
  std::unique_lock<std::mutex> queue_lock(queue_mtx_, std::defer_lock);
  const auto queue_locked = queue_lock.try_lock();
  if(queue_locked)
  {
    do_task_impl(queue_lock);
  }
}

///
///
auto task_queue::do_task_or_wait() -> void
{
  auto queue_lock = std::unique_lock<std::mutex>(queue_mtx_);
  if(task_queue_.empty() && !shutdown_)
  {
    task_cv_.wait(queue_lock, [this] { return !task_queue_.empty() || shutdown_; });
  }
  do_task_impl(queue_lock);
}

///
///
auto task_queue::do_task_impl(std::unique_lock<std::mutex>& queue_lock) -> void
{
  if(!task_queue_.empty())
  {
    auto task = std::move(task_queue_.front());
    task_queue_.pop();
    queue_lock.unlock();
    if(task)
    {
      const auto task_lock = std::scoped_lock{task_mtx_};
      task();
    }
  }
}

} // namespace bibstd::framework
