#include "bibstd/framework/thread_pool.hpp"
#include "bibstd/util/contains.hpp"
#include "bibstd/util/exception.hpp"

#include <algorithm>
#include <mutex>
#include <ranges>
#include <utility>

namespace bibstd::framework
{

///
///
auto thread_pool::strand_id() -> strand_id_type
{
  return strand_id_type::new_uid();
}

///
///
auto thread_pool::init() -> util::shared_scope_guard
{
  static util::shared_scope_guard::creator guard_creator{};
  static std::condition_variable cv_init{};

  auto lock = std::unique_lock{mtx_};
  auto guard = guard_creator.create(
    []()
    {
      initialized_ = false;
      // Make sure the lock is not used by anyone else anymore. The initialized flag ensures,
      // that the pool_ member is not modified or read from external threads anymore.
      auto pool = pool_type{};
      {
        const auto lock = std::scoped_lock{mtx_};
        pool = std::exchange(pool_, {});
      }
      // Joined outside of the lock, a running task takes it once it is done
      pool.clear();
      cv_init.notify_all();
    }
  );
  if(guard.is_initial_instance())
  {
    if(!pool_.empty())
    {
      cv_init.wait(lock, []() { return pool_.empty(); });
    }
    initialized_ = true;
    pool_.emplace_back(std::make_unique<pool_element>());
  }
  return guard;
}

///
///
auto thread_pool::queue_task(task_type&& task) -> void
{
  if(!initialized_)
  {
    throw util::exception("thread pool not initialized");
  }
  auto lock = std::unique_lock{mtx_};
  static const auto internal_strand_id = strand_id();
  queue_task_auto(task_data{.task = std::move(task), .task_id = task_id_type::new_uid(), .strand_id = internal_strand_id});
  auto abandoned = extract_abandoned_workers();
  lock.unlock();
  abandoned.clear();
}

///
///
auto thread_pool::queue_task(task_type&& task, const strand_id_type id) -> void
{
  if(!initialized_)
  {
    throw util::exception("thread pool not initialized");
  }
  auto lock = std::unique_lock{mtx_};
  const auto dest_thread = std::ranges::find_if(
    pool_, [&](const auto& e) { return util::contains(e->ids, [id](const auto& p) { return p.strand_id == id; }); }
  );
  const auto index = static_cast<std::size_t>(std::ranges::distance(std::ranges::cbegin(pool_), dest_thread));
  if(index < pool_.size())
  {
    queue_task_index(task_data{.task = std::move(task), .task_id = task_id_type::new_uid(), .strand_id = id}, index);
  }
  else
  {
    queue_task_auto(task_data{.task = std::move(task), .task_id = task_id_type::new_uid(), .strand_id = id});
  }
  auto abandoned = extract_abandoned_workers();
  lock.unlock();
  abandoned.clear();
}

///
///
auto thread_pool::queue_task_index(task_data&& data, const std::size_t index) -> void
{
  decltype(auto) element = pool_.at(index);
  element->ids.emplace_back(id_pair{.task_id = data.task_id, .strand_id = data.strand_id});
  element->worker.queue_task(create_task_wrapper(std::move(data), element.get()));
}

///
///
auto thread_pool::queue_task_auto(task_data&& data) -> void
{
  const auto iter = std::ranges::find_if(pool_, [&](const auto& e) { return e->ids.empty(); });
  auto index = static_cast<std::size_t>(std::ranges::distance(std::ranges::cbegin(pool_), iter));
  if(iter == std::ranges::cend(pool_))
  {
    pool_.emplace_back(std::make_unique<pool_element>());
    index = pool_.size() - 1;
  }
  decltype(auto) element = pool_.at(index);
  element->ids.emplace_back(id_pair{.task_id = data.task_id, .strand_id = data.strand_id});
  element->worker.queue_task(create_task_wrapper(std::move(data), element.get()));
}

///
///
auto thread_pool::create_task_wrapper(task_data&& data, const util::non_owning_ptr<pool_element> element) -> task_type
{
  element->last_use = std::chrono::system_clock::now();
  return [forwarded_data = std::move(data), element]() mutable
  {
    // These locks can be called during uninitialized state and the pool will be modified. Since it is ensured that the worker
    // calling this function will be destroyed before the pool element (including the ids), the destruction is well defined.
    auto pre_lock = std::unique_lock(mtx_);
    const auto found = util::contains(element->ids, [&](const auto& p) { return p.task_id == forwarded_data.task_id; });
    pre_lock.unlock();
    if(found && forwarded_data.task)
    {
      forwarded_data.task();
    }
    const auto post_lock = std::scoped_lock{mtx_};
    std::erase_if(element->ids, [&](const auto& p) { return p.task_id == forwarded_data.task_id; });
  };
}

///
///
auto thread_pool::extract_abandoned_workers() -> pool_type
{
  using ms = std::chrono::milliseconds;
  const auto now = std::chrono::system_clock::now();
  pool_type abandoned;
  std::ranges::for_each(
    pool_,
    [&](auto& element)
    {
      const auto empty_ids = element->ids.empty();
      const auto inactive_duration = now > element->last_use ? std::chrono::duration_cast<ms>(now - element->last_use) : ms{0};
      if(empty_ids && inactive_duration > std::chrono::minutes{1})
      {
        abandoned.emplace_back(std::move(element));
        element = nullptr;
      }
    }
  );
  std::erase_if(pool_, [](const auto& element) { return element == nullptr; });
  return abandoned;
}

} // namespace bibstd::framework
