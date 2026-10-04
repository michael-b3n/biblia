#include "bibstd/workflow/workflow_template.hpp"

#include <tuple>

namespace bibstd::workflow
{

///
///
workflow_template_settings::workflow_template_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : framework::settings_base{std::move(workflow_settings)} // clang-format off
  // , plain_bool{workflow_settings_->create_setting("template.plain_bool", true)}
  // , plain_int(workflow_settings_->create_setting("template.plain_int", 42))
  // , plain_double(workflow_settings_->create_setting("template.plain_double", 42.42))
  // , plain_string(workflow_settings_->create_setting("template.plain_string", std::string{"test"}))
  // , optional_bool(workflow_settings_->create_setting("template.optional_bool", setting_value_t<decltype(optional_bool)>{}))
  // , optional_int(workflow_settings_->create_setting("template.optional_int", setting_value_t<decltype(optional_int)>{42}))
  // , optional_double(workflow_settings_->create_setting("template.optional_double", setting_value_t<decltype(optional_double)>{42.42}))
  // , optional_string(workflow_settings_->create_setting("template.optional_string", setting_value_t<decltype(optional_string)>{}))
  // , vector_string(workflow_settings_->create_setting("template.vector_string", setting_value_t<decltype(vector_string)>{std::string{"test1"}}))
// clang-format on
{
}

///
///
workflow_template::workflow_template(
  std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script
)
  : workflow_base{std::move(workflow_settings)}
  , workflow_script_{std::move(workflow_script)}
{
  // Last, scripts may call into the workflow from here on. A failed registration is logged by the state.
  registrations_ << workflow_script_->register_function(
    "template.start", [this](int value) { return start(params{{.value = value}}).has_value(); }
  );
  // registrations_ << workflow_script_->register_setting(*settings().plain_bool);
}

///
///
workflow_template::~workflow_template() noexcept = default;

///
///
auto workflow_template::start([[maybe_unused]] const params& /*params*/) -> result
{
  // do something
  return return_failure;
}

} // namespace bibstd::workflow
