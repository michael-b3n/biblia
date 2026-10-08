#pragma once

#include "bibstd/framework/process_params.hpp"
#include "bibstd/workflow/workflow_settings.hpp"
#include "bibstd/workflow/workflow_settings_base.hpp"

#include <concepts>
#include <expected>
#include <memory>
#include <type_traits>

namespace bibstd::workflow
{

///
/// Base class for workflows.
///
struct workflow_ground
{
  // Constants
  static constexpr std::unexpected return_failure{framework::process_result_unexpected::failure};
  static constexpr std::unexpected return_stopped{framework::process_result_unexpected::stopped};

  // Structors
  virtual ~workflow_ground() noexcept = default;
};

template<typename T>
class workflow_base;

///
/// template specialization \see workflow_base
///
template<>
class workflow_base<void> : public workflow_ground
{
protected: // Structors
  workflow_base() = default;
  ~workflow_base() noexcept override = default;
};

///
/// template specialization \see workflow_base
///
template<std::derived_from<workflow_settings_base> S>
class workflow_base<S> : public workflow_ground
{
protected: // Structors
  workflow_base(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_base() noexcept override = default;

public: // Typedefs
  using settings_type = std::conditional_t<std::is_void_v<S>, void, std::unique_ptr<S>>;

public: // Accessors
  ///
  /// \return const reference to settings container
  ///
  auto settings() const -> const S&;

private: // Variables
  const settings_type settings_;
};

///
///
template<std::derived_from<workflow_settings_base> S>
workflow_base<S>::workflow_base(std::shared_ptr<workflow_settings> workflow_settings)
  : settings_{std::make_unique<S>(std::move(workflow_settings))}
{
}

///
///
template<std::derived_from<workflow_settings_base> S>
auto workflow_base<S>::settings() const -> const S&
{
  return *settings_;
}

} // namespace bibstd::workflow
