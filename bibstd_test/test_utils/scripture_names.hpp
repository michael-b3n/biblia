#pragma once

#include <arcstd/framework/setting_validator.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace bibstd::test_utils
{

///
/// The scriptures of the app may come from bundled scripts too, so a test counts those of its own scripts.
/// \return the names of the scriptures \p scripture offers from the script named \p script
///
[[nodiscard]] inline auto scripture_names_of(const workflow::workflow_scripture& scripture, const std::string_view script)
  -> std::vector<std::string>
{
  using validator_type = arcstd::framework::setting_validator_list<std::optional<std::string>>::sptr_type;
  const auto suffix = std::format(" ({})", script);
  return std::get<validator_type>(scripture.settings().default_scripture_name->validator)->available() |
         std::views::filter([&](const auto& name) { return name.ends_with(suffix); }) | std::ranges::to<std::vector>();
}

} // namespace bibstd::test_utils
