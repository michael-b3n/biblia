#pragma once

#include <QProcess>
#include <QTimer>

#include <chrono>
#include <memory>
#include <optional>

namespace Velopack
{
class UpdateManager;
} // namespace Velopack

namespace bibqml
{
class BridgeApplication;
} // namespace bibqml

namespace verselens
{

///
/// Updater: checks the GitHub releases for a new version while the application runs and downloads it.
/// The check and the download run in a process of their own, see run_update_download(). Destroying the updater ends
/// that process, a download it did not finish is discarded.
///
class app_velopack_updater final
{
  // Constants
  static constexpr auto first_check_delay = std::chrono::minutes{3};
  static constexpr auto check_interval = std::chrono::hours{24};
  static constexpr auto retry_interval = std::chrono::hours{6};
  static constexpr auto download_timeout = std::chrono::hours{2};

  // Variables
  bibqml::BridgeApplication& bridge_;
  std::unique_ptr<Velopack::UpdateManager> manager_;
  bool install_started_{false};
  QTimer timer_;
  QProcess download_;

public: // Structors
  ///
  /// Start checking for updates. A downloaded update is reported
  /// to \p bridge, which has to outlive the updater.
  ///
  explicit app_velopack_updater(bibqml::BridgeApplication& bridge);
  ~app_velopack_updater() noexcept;
  app_velopack_updater(const app_velopack_updater&) = delete;
  app_velopack_updater(app_velopack_updater&&) = delete;
  auto operator=(const app_velopack_updater&) -> app_velopack_updater& = delete;
  auto operator=(app_velopack_updater&&) -> app_velopack_updater& = delete;

public: // Modifiers
  ///
  /// Check for an update right away, unless a check is running already.
  ///
  auto check_now() -> void;

  ///
  /// Install the downloaded update, see verselens::install_downloaded_update.
  /// \return true if the update was handed over, which happens once
  ///
  [[nodiscard]] auto install_and_restart() -> bool;

private: // Implementation
  auto timer_elapsed() -> void;
  auto download_finished(int exit_code, QProcess::ExitStatus exit_status) -> void;
};

///
/// Check for an update and download it, if the updater started this process to do so.
/// \return exit code of the process, std::nullopt if this process runs the application
///
[[nodiscard]] auto run_update_download(int argc, char** argv) -> std::optional<int>;

///
/// Hand the downloaded update to Velopack. It waits for this process to exit, installs the update and starts the
/// application again, so the caller has to quit the application next.
/// \return true if the update was handed over, false if there is none or this executable was not installed by Velopack
///
[[nodiscard]] auto install_downloaded_update() -> bool;

} // namespace verselens
