#include "bibstd/workflow/workflow_web.hpp"
#include "bibstd/core/core_fetch_web_content.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/exception.hpp"

#include <optional>
#include <tuple>
#include <utility>

namespace bibstd::workflow
{

///
///
workflow_web::workflow_web(std::shared_ptr<workflow_script> workflow_script)
  : workflow_script_{std::move(workflow_script)}
{
  // The page, or nil and why it failed
  registrations_ << workflow_script_->register_function("web.fetch", [this](const std::string& url) { return fetch(url); });
}

///
///
workflow_web::~workflow_web() noexcept = default;

///
///
auto workflow_web::fetch(const std::string_view url) const -> page_type
{
  try
  {
    return core::core_fetch_web_content::fetch(url).transform_error([](const auto error)
                                                                    { return std::string{util::enum_name(error)}; });
  }
  catch(...)
  {
    return std::unexpected{util::exception_report()};
  }
}

} // namespace bibstd::workflow
