#pragma once

#include "bibstd/framework/process_params.hpp"
#include "bibstd/lua/registration.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_script.hpp"
#include "bibstd/workflow/workflow_settings_base.hpp"

#include <memory>

namespace bibstd::workflow
{

///
/// Settings corresponding to workflow template.
///
class workflow_template_settings final : public workflow_settings_base
{
public: // Structors
  workflow_template_settings(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_template_settings() noexcept override = default;

  // clang-format off
public: // Variables
  // const setting_type<bool> plain_bool;
  // const setting_type<int> plain_int;
  // const setting_type<double> plain_double;
  // const setting_type<std::string> plain_string;
  // const setting_type<std::optional<bool>> optional_bool;
  // const setting_type<std::optional<int>> optional_int;
  // const setting_type<std::optional<double>> optional_double;
  // const setting_type<std::optional<std::string>> optional_string;
  // const setting_type<std::vector<std::string>> vector_string;
};
// clang-format on

///
/// Workflow template. This is a template for creating new workflows.
/// A workflow without Lua registrations leaves out workflow_script.
///
class workflow_template final : public workflow_base<workflow_template_settings>
{
  // Typedefs
  struct params_t final
  {
    int value{0}; /*some type*/
  };
  using result_t = float /*some type*/;

  // Variables
  const std::shared_ptr<workflow_script> workflow_script_;
  lua::registration registrations_;

public: // Typedefs
  using params = framework::process_params<params_t>;
  using result = framework::process_result<result_t>;

public: // Structors
  workflow_template(std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script);
  ~workflow_template() noexcept override;

public: // Modifiers
  ///
  /// Start the workflow template.
  /// \return result, or an unexpected result in case of failure
  ///
  auto start(const params& params) -> result;
};

} // namespace bibstd::workflow
