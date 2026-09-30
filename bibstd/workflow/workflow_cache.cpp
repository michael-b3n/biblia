#include "bibstd/workflow/workflow_cache.hpp"
#include "bibstd/core/core_cache.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/identifier.hpp"

#include <format>
#include <optional>
#include <tuple>
#include <utility>

namespace bibstd::workflow
{

///
///
workflow_cache::workflow_cache(std::shared_ptr<workflow_script> workflow_script)
  : workflow_script_{std::move(workflow_script)}
  , folder_{workflow_script_->settings().folder->value() / workflow_script_settings::cache_folder_name}
{
  // Last, scripts may call into the workflow from here on
  std::ignore = workflow_script_->register_function(
    "cache.get", [this](const std::string& name, const std::string& key) { return get(util::identifier{name}, key); }
  );
  std::ignore = workflow_script_->register_function(
    "cache.set",
    [this](const std::string& name, const std::string& key, std::optional<std::string> value)
    { set(util::identifier{name}, key, std::move(value)); }
  );
}

///
///
workflow_cache::~workflow_cache() noexcept
{
  // First, so no script calls into the workflow once its destruction starts. For good, it lives as long as the app.
  workflow_script_->shutdown();
}

///
///
auto workflow_cache::get(const util::identifier& name, const std::string& key) -> std::optional<std::string>
{
  return cache(name).get(key);
}

///
///
auto workflow_cache::set(const util::identifier& name, const std::string& key, std::optional<std::string> value) -> void
{
  cache(name).set(key, std::move(value));
}

///
///
auto workflow_cache::cache(const util::identifier& name) -> core::core_cache&
{
  const auto lock = std::scoped_lock{mtx_};
  auto& cache = caches_[name.string()];
  if(!cache)
  {
    cache = std::make_unique<core::core_cache>(folder_ / std::format("{}.sqlite", name.string()));
  }
  return *cache;
}

} // namespace bibstd::workflow
