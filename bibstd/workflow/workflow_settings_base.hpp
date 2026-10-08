#pragma once

#include "bibstd/workflow/workflow_settings.hpp"

#include <memory>
#include <type_traits>

namespace bibstd::workflow
{

///
/// Base class of the settings of a workflow.
///
class workflow_settings_base
{
public: // Structors
  workflow_settings_base(std::shared_ptr<workflow_settings> workflow_settings);
  virtual ~workflow_settings_base() noexcept;
  workflow_settings_base(const workflow_settings_base&) = delete;
  workflow_settings_base(workflow_settings_base&&) = delete;
  auto operator=(const workflow_settings_base&) -> workflow_settings_base& = delete;
  auto operator=(workflow_settings_base&&) -> workflow_settings_base& = delete;

protected: // Typedefs
  template<typename T>
  using setting_value_t = typename std::remove_cv_t<std::remove_pointer_t<T>>::value_type;

  template<typename T>
  using setting_type = workflow_settings::setting_non_owning_ptr_type<T>;

protected: // Variables
  std::shared_ptr<workflow_settings> workflow_settings_;
};

} // namespace bibstd::workflow
