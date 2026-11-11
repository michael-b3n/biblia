#include "arcstd/workflow/workflow_settings_base.hpp"

namespace arcstd::workflow
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

} // namespace arcstd::workflow
