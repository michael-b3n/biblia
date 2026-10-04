#pragma once

#include "bibstd/lua/registration.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_script.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>

namespace bibstd::workflow
{

///
/// Workflow web, fetches web pages. Registered for scripts as workflow.web.fetch, they parse the page themselves.
///
class workflow_web final : public workflow_base<void>
{
public: // Typedefs
  using page_type = std::expected<std::string, std::string>;

private: // Variables
  const std::shared_ptr<workflow_script> workflow_script_;
  lua::registration registrations_;

public: // Structors
  workflow_web(std::shared_ptr<workflow_script> workflow_script);
  ~workflow_web() noexcept override;

public: // Operations
  ///
  /// \return the web page at \p url, or why it failed. Blocks until it is fetched.
  ///
  [[nodiscard]] auto fetch(std::string_view url) const -> page_type;
};

} // namespace bibstd::workflow
