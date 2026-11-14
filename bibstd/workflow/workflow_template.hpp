#pragma once

#include "bibstd/workflow/process_params.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_settings_base.hpp"

#include <arcstd/lua/lua_engine.hpp>
#include <arcstd/lua/registration.hpp>

#include <memory>

namespace bibstd::workflow
{

///
/// Settings corresponding to workflow template.
///
class workflow_template_settings final : public workflow_settings_base
{
public: // Structors
  workflow_template_settings(std::shared_ptr<arcstd::setting::setting_store> setting_store);
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
/// A workflow without Lua registrations leaves out lua_engine.
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
  const std::shared_ptr<arcstd::lua::lua_engine> lua_engine_;
  arcstd::lua::registration registrations_;

public: // Typedefs
  using params = process_params<params_t>;
  using result = process_result<result_t>;

public: // Structors
  workflow_template(
    std::shared_ptr<arcstd::setting::setting_store> setting_store, std::shared_ptr<arcstd::lua::lua_engine> lua_engine
  );
  ~workflow_template() noexcept override;

public: // Modifiers
  ///
  /// Start the workflow template.
  /// \return result, or an unexpected result in case of failure
  ///
  [[nodiscard]] auto start(const params& params) -> result;
};

} // namespace bibstd::workflow
