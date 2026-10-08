#include "bibstd/workflow/workflow_web.hpp"
#include "bibstd/core/core_fetch_web_content.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/exception.hpp"

#include <utility>

namespace bibstd::workflow
{

///
///
workflow_web_settings::workflow_web_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : workflow_settings_base{std::move(workflow_settings)}
  , retry_pause{workflow_settings_->create_setting(
      "web.retry_pause",
      std::chrono::seconds{60},
      std::make_shared<framework::setting_validator_range<std::chrono::seconds>>(
        std::chrono::seconds{0}, std::chrono::hours{24}
      )
    )}
{
}

///
///
workflow_web::workflow_web(
  std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script
)
  : workflow_base{std::move(workflow_settings)}
  , workflow_script_{std::move(workflow_script)}
{
  registrations_ << workflow_script_->register_function("web.fetch", [this](const std::string& url) { return fetch(url); });
}

///
///
workflow_web::~workflow_web() noexcept = default;

///
///
auto workflow_web::fetch(const std::string_view url) const -> page_type
{
  using clock_type = std::chrono::steady_clock;
  try
  {
    {
      const auto lock = std::scoped_lock{mtx_};
      std::erase_if(failures_, [now = clock_type::now()](const auto& failure) { return failure.second.retry_at <= now; });
      if(const auto failed = failures_.find(std::string{url}); failed != failures_.cend())
      {
        return std::unexpected{failed->second.reason};
      }
    }
    // Without the lock, the request may take until its timeout
    auto page = core::core_fetch_web_content::fetch(url).transform_error([](const auto error)
                                                                         { return std::string{util::enum_name(error)}; });
    if(!page)
    {
      const auto lock = std::scoped_lock{mtx_};
      failures_.insert_or_assign(
        std::string{url}, failure_t{.reason = page.error(), .retry_at = clock_type::now() + settings().retry_pause->value()}
      );
    }
    return page;
  }
  catch(...)
  {
    return std::unexpected{util::exception_report()};
  }
}

} // namespace bibstd::workflow
