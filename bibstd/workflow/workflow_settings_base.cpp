#include "bibstd/workflow/workflow_settings_base.hpp"

namespace bibstd::workflow
{

///
///
workflow_settings_base::workflow_settings_base(std::shared_ptr<arcstd::setting::setting_store> setting_store)
  : setting_store_{std::move(setting_store)}
{
}

///
///
workflow_settings_base::~workflow_settings_base() noexcept = default;

} // namespace bibstd::workflow
