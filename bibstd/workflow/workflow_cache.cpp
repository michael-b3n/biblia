#include "bibstd/workflow/workflow_cache.hpp"
#include "bibstd/core/core_cache.hpp"
#include "bibstd/util/identifier.hpp"

#include <format>
#include <optional>
#include <utility>

namespace bibstd::workflow
{

///
///
workflow_cache::workflow_cache(
  std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script
)
  : workflow_script_{std::move(workflow_script)}
  , folder_{workflow_settings->data_folder() / folder_name}
{
  registrations_
    ///
    /// \lua Reads a text value kept beyond the run of the app. Each cache is the SQLite file `cache/<name>.sqlite` of the
    /// local data folder, other programs may read and change it meanwhile. A failing cache is logged and holds nothing.
    /// \param name the cache, of letters, digits, `_` and `-`, a script names its own
    /// \param key the key of the value
    /// \return the value, `nil` for none
    ///
    << workflow_script_->register_function(
         "cache.get", [this](const std::string& name, const std::string& key) { return get(util::identifier{name}, key); }
       )
    ///
    /// \lua Writes a text value to a cache, \see `workflow.cache.get`.
    /// \param name the cache
    /// \param key the key of the value
    /// \param value the value, `nil` removes the key
    ///
    << workflow_script_->register_function(
         "cache.set",
         [this](const std::string& name, const std::string& key, std::optional<std::string> value)
         { set(util::identifier{name}, key, std::move(value)); }
       );
}

///
///
workflow_cache::~workflow_cache() noexcept = default;

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
