#pragma once

#include "bibstd/lua/registration.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_script.hpp"
#include "bibstd/workflow/workflow_settings.hpp"
#include "bibstd/workflow/workflow_settings_base.hpp"

#include <chrono>
#include <expected>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace bibstd::workflow
{

///
/// Settings corresponding to workflow web.
///
class workflow_web_settings final : public workflow_settings_base
{
public: // Structors
  workflow_web_settings(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_web_settings() noexcept override = default;

public: // Variables
  const setting_type<std::chrono::seconds> retry_pause;
};

///
/// Workflow web, fetches web pages. Registered for scripts as workflow.web.fetch, they parse the page themselves.
/// A url that failed is not requested again before the setting "web.retry_pause" is over,
/// 60 s by default: a script may ask for it once per verse.
///
class workflow_web final : public workflow_base<workflow_web_settings>
{
  // Typedefs
  struct failure_t final
  {
    std::string reason;
    std::chrono::steady_clock::time_point retry_at;
  };

  // Variables
  const std::shared_ptr<workflow_script> workflow_script_;
  mutable std::mutex mtx_;
  mutable std::map<std::string, failure_t> failures_;
  lua::registration registrations_;

public: // Typedefs
  using page_type = std::expected<std::string, std::string>;

public: // Structors
  workflow_web(std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script);
  ~workflow_web() noexcept override;

public: // Operations
  ///
  /// \return the web page at \p url, or why it failed: the name of a core_fetch_web_content::error_code.
  /// Blocks until it is fetched, a failure is answered from memory during the retry pause.
  ///
  [[nodiscard]] auto fetch(std::string_view url) const -> page_type;
};

} // namespace bibstd::workflow
