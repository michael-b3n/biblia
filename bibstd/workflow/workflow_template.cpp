#include "bibstd/workflow/workflow_template.hpp"

namespace bibstd::workflow
{

///
///
workflow_template_settings::workflow_template_settings(std::shared_ptr<arcstd::setting::setting_store> setting_store)
  : workflow_settings_base{std::move(setting_store)} // clang-format off
  // , plain_bool{setting_store_->create_setting("template.plain_bool", true)}
  // , plain_int(setting_store_->create_setting("template.plain_int", 42))
  // , plain_double(setting_store_->create_setting("template.plain_double", 42.42))
  // , plain_string(setting_store_->create_setting("template.plain_string", std::string{"test"}))
  // , optional_bool(setting_store_->create_setting("template.optional_bool", setting_value_t<decltype(optional_bool)>{}))
  // , optional_int(setting_store_->create_setting("template.optional_int", setting_value_t<decltype(optional_int)>{42}))
  // , optional_double(setting_store_->create_setting("template.optional_double", setting_value_t<decltype(optional_double)>{42.42}))
  // , optional_string(setting_store_->create_setting("template.optional_string", setting_value_t<decltype(optional_string)>{}))
  // , vector_string(setting_store_->create_setting("template.vector_string", setting_value_t<decltype(vector_string)>{std::string{"test1"}}))
// clang-format on
{
}

///
///
workflow_template::workflow_template(
  std::shared_ptr<arcstd::setting::setting_store> setting_store, std::shared_ptr<arcstd::lua::lua_engine> lua_engine
)
  : workflow_base{std::move(setting_store)}
  , lua_engine_{std::move(lua_engine)}
{
  // registrations_ << lua_engine_->register_function(
  //   "template.start", [this](int value) { return start(params{{.value = value}}).has_value(); }
  // );
  // registrations_ << lua_engine_->register_setting(*settings().plain_bool);
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
