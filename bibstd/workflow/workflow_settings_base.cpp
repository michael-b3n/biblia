#include "bibstd/workflow/workflow_settings_base.hpp"

namespace bibstd::workflow
{

///
///
workflow_settings_base::workflow_settings_base(std::shared_ptr<workflow_settings> workflow_settings)
  : workflow_settings_{std::move(workflow_settings)}
{
}

///
///
workflow_settings_base::~workflow_settings_base() noexcept = default;

} // namespace bibstd::workflow
