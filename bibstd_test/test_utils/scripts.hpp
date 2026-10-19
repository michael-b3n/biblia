#pragma once

#include <bibstd/framework/setting_validator.hpp>
#include <bibstd/workflow/workflow_script.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>
#include <bibstd/workflow/workflow_settings.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <format>
#include <future>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace bibstd::test_utils
{

///
/// Settings in \p data_folder with the script settings written before a workflow reads them. The user scripts
/// are the folder "scripts" in it.
/// An absolute folder name replaces the local data folder, so the tests leave it alone.
///
[[nodiscard]] inline auto make_script_settings(const std::filesystem::path& data_folder, const bool enabled = false)
  -> std::shared_ptr<workflow::workflow_settings>
{
  {
    const auto writer = std::make_shared<workflow::workflow_settings>(data_folder.string());
    const auto settings = workflow::workflow_script_settings{writer};
    std::ignore = settings.enabled->value(enabled);
  }
  return std::make_shared<workflow::workflow_settings>(data_folder.string());
}

///
/// Load the scripts of \p workflow and wait for scripts_loaded.
///
inline auto load_scripts(workflow::workflow_script& workflow) -> void
{
  const auto loaded = std::make_shared<std::promise<void>>();
  auto done = loaded->get_future();
  const auto connection =
    workflow.connect(&workflow::workflow_script_sigs::scripts_loaded, [loaded]() { loaded->set_value(); });
  workflow.load_scripts();
  REQUIRE(done.wait_for(std::chrono::seconds{10}) == std::future_status::ready);
}

///
/// The scriptures of the app may come from bundled scripts too, so a test counts those of its own scripts.
/// \return the names of the scriptures \p scripture offers from the script named \p script
///
[[nodiscard]] inline auto scripture_names_of(const workflow::workflow_scripture& scripture, const std::string_view script)
  -> std::vector<std::string>
{
  using validator_type = framework::setting_validator_list<std::optional<std::string>>::sptr_type;
  const auto suffix = std::format(" ({})", script);
  return std::get<validator_type>(scripture.settings().default_scripture_name->validator)->available() |
         std::views::filter([&](const auto& name) { return name.ends_with(suffix); }) | std::ranges::to<std::vector>();
}

} // namespace bibstd::test_utils
