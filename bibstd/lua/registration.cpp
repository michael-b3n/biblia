#include "bibstd/lua/registration.hpp"
#include "bibstd/lua/names.hpp"
#include "bibstd/lua/state.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"

#include <algorithm>
#include <iterator>
#include <mutex>
#include <ranges>
#include <utility>

namespace bibstd::lua
{

///
///
registration::registration(std::weak_ptr<detail::state_data> data, util::path path, std::shared_ptr<const void> registered)
{
  entries_.push_back({.data = std::move(data), .path = std::move(path), .registered = std::move(registered)});
}

///
///
registration::registration(registration&& other) noexcept
  : entries_{std::exchange(other.entries_, {})}
{
}

///
///
registration::~registration() noexcept
{
  release();
}

///
///
auto registration::operator=(registration&& other) noexcept -> registration&
{
  if(this != &other)
  {
    release();
    entries_ = std::exchange(other.entries_, {});
  }
  return *this;
}

///
///
auto registration::operator<<(registration other) -> registration&
{
  std::ranges::move(std::exchange(other.entries_, {}), std::back_inserter(entries_));
  return *this;
}

///
///
registration::operator bool() const noexcept
{
  return !entries_.empty();
}

///
///
auto registration::release() noexcept -> void
{
  std::ranges::for_each(
    std::exchange(entries_, {}) | std::views::reverse,
    [](entry& e)
    {
      const auto data = e.data.lock();
      if(!data)
      {
        return;
      }
      try
      {
        const auto lock = std::scoped_lock{data->mtx};
        e.registered.reset();
        if(!data->shutdown_flag)
        {
          const auto remove = data->lua.traverse_get<sol::protected_function>(
            names::node_root, names::node_system, names::node_util, names::function_remove
          );
          if(const auto result = remove(e.path.string()); !result.valid())
          {
            const sol::error error = result;
            LOG_ERROR("lua registration not removed: path=\"{}\", {}", e.path.string(), error.what());
          }
        }
      }
      catch(...)
      {
        LOG_ERROR("exception occurred: {}", util::exception_report());
      }
    }
  );
}

} // namespace bibstd::lua
