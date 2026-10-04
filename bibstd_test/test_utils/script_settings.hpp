#pragma once

#include <bibstd/workflow/workflow_script.hpp>
#include <bibstd/workflow/workflow_settings.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <tuple>

namespace bibstd::test_utils
{

///
/// Settings in \p data_folder with the script settings written before a workflow reads them.
/// An absolute folder name replaces the local data folder, so the tests leave it alone.
///
[[nodiscard]] inline auto make_script_settings(
  const std::filesystem::path& data_folder, const bool enabled = false, const std::filesystem::path& scripts = {}
) -> std::shared_ptr<workflow::workflow_settings>
{
  {
    const auto writer = std::make_shared<workflow::workflow_settings>(data_folder.string());
    const auto settings = workflow::workflow_script_settings{writer};
    std::ignore = settings.enabled->value(enabled);
    std::ignore = settings.folder->value(scripts.empty() ? data_folder / "scripts" : scripts);
  }
  return std::make_shared<workflow::workflow_settings>(data_folder.string());
}

///
/// Wait until \p condition holds, the workflows update on threads of their own.
/// \return false if it does not within a few seconds
///
[[nodiscard]] inline auto wait_until(const std::function<bool()>& condition) -> bool
{
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
  while(!condition())
  {
    if(std::chrono::steady_clock::now() > deadline)
    {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  return true;
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

} // namespace bibstd::test_utils
