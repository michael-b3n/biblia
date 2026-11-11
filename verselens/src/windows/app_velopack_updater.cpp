#include "src/windows/app_velopack_updater.hpp"
#include "res/version.hpp"

#include <arcqml/bridge/BridgeApplication.hpp>

#include <arcstd/util/exception.hpp>
#include <arcstd/util/log.hpp>

#include <Velopack.hpp>

#include <QCoreApplication>
#include <QStringList>

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace verselens
{
namespace
{

// Argument the updater starts this executable with to check for an update and download it
constexpr auto download_argument = std::string_view{"--download-update"};

///
/// Create the manager of the updates of this installation.
/// \throws Velopack exception if this executable was not installed by Velopack
///
[[nodiscard]] auto make_update_manager() -> std::unique_ptr<Velopack::UpdateManager>
{
  return std::make_unique<Velopack::UpdateManager>(
    std::make_unique<Velopack::GithubSource>(std::string{version::repository_url})
  );
}

} // namespace

///
///
app_velopack_updater::app_velopack_updater(bibqml::BridgeApplication& bridge)
  : bridge_{bridge}
{
  try
  {
    manager_ = make_update_manager();
  }
  catch(...)
  {
    // Started from the build folder
    LOG_INFO("update checks inactive: {}", bibstd::util::exception_report());
    return;
  }
  bridge_.notifyUpdateCheck(bibqml::BridgeApplication::UpdateCheckIdle);
  timer_.setSingleShot(true);
  download_.setProcessChannelMode(QProcess::MergedChannels);
  QObject::connect(&timer_, &QTimer::timeout, &timer_, [this]() { timer_elapsed(); });
  QObject::connect(
    &download_,
    &QProcess::finished,
    &download_,
    [this](const int exit_code, const QProcess::ExitStatus exit_status) { download_finished(exit_code, exit_status); }
  );
  QObject::connect(
    &download_,
    &QProcess::errorOccurred,
    &download_,
    [this](const QProcess::ProcessError error)
    {
      // A process that started reports its end through finished
      if(error == QProcess::FailedToStart)
      {
        LOG_WARN("update check failed to start: {}", download_.errorString().toStdString());
        bridge_.notifyUpdateCheck(bibqml::BridgeApplication::UpdateCheckFailed);
        timer_.start(retry_interval);
      }
    }
  );
  timer_.start(first_check_delay);
}

///
///
app_velopack_updater::~app_velopack_updater() noexcept
{
  // A download in flight ends here and is not reported as a failed check
  QObject::disconnect(&download_, nullptr, nullptr, nullptr);
  download_.kill();
  download_.waitForFinished();
}

///
///
auto app_velopack_updater::check_now() -> void
{
  // Inactive updater, running check or an install on its way
  if(manager_ == nullptr || download_.state() != QProcess::NotRunning || install_started_)
  {
    return;
  }
  timer_elapsed();
}

///
///
auto app_velopack_updater::install_and_restart() -> bool
{
  // A second request before the application quit would start a second install
  if(install_started_)
  {
    return false;
  }
  install_started_ = install_downloaded_update();
  return install_started_;
}

///
///
auto app_velopack_updater::timer_elapsed() -> void
{
  if(download_.state() != QProcess::NotRunning)
  {
    // Reported through finished
    LOG_WARN("update check timed out");
    download_.kill();
    return;
  }
  // Started first, a process that fails to start sets the retry
  timer_.start(download_timeout);
  bridge_.notifyUpdateCheck(bibqml::BridgeApplication::UpdateCheckRunning);
  download_.start(QCoreApplication::applicationFilePath(), QStringList{QString::fromUtf8(download_argument)});
}

///
///
auto app_velopack_updater::download_finished(const int exit_code, const QProcess::ExitStatus exit_status) -> void
{
  if(exit_status != QProcess::NormalExit || exit_code != EXIT_SUCCESS)
  {
    // Offline or GitHub not reachable
    LOG_WARN("update check failed: exit_code={}, output=\"{}\"", exit_code, download_.readAll().trimmed().toStdString());
    bridge_.notifyUpdateCheck(bibqml::BridgeApplication::UpdateCheckFailed);
    timer_.start(retry_interval);
    return;
  }
  bridge_.notifyUpdateCheck(bibqml::BridgeApplication::UpdateCheckFinished);
  timer_.start(check_interval);
  if(const auto pending = manager_->UpdatePendingRestart(); pending.has_value())
  {
    LOG_INFO("update downloaded: version={}", pending->Version);
    bridge_.notifyUpdateAvailable();
  }
  else
  {
    LOG_INFO("no update available");
  }
}

///
///
auto run_update_download(const int argc, char** argv) -> std::optional<int>
{
  const auto args = std::span{argv, static_cast<std::size_t>(argc)};
  if(args.size() != 2 || std::string_view{args[1]} != download_argument)
  {
    return std::nullopt;
  }
  // The log file belongs to the application, the updater logs the output of this process instead
  try
  {
    const auto manager = make_update_manager();
    if(const auto update = manager->CheckForUpdates(); update.has_value())
    {
      // Returns right away if the update is downloaded already
      manager->DownloadUpdates(*update);
    }
    return EXIT_SUCCESS;
  }
  catch(...)
  {
    std::cout << bibstd::util::exception_report();
    return EXIT_FAILURE;
  }
}

///
///
auto install_downloaded_update() -> bool
{
  auto manager = std::unique_ptr<Velopack::UpdateManager>{};
  try
  {
    manager = make_update_manager();
  }
  catch(...)
  {
    // Started from the build folder, the updater reports it
    return false;
  }
  const auto pending = manager->UpdatePendingRestart();
  if(!pending)
  {
    return false;
  }
  try
  {
    manager->WaitExitThenApplyUpdates(*pending, true /*silent*/, true /*restart*/);
    LOG_INFO("update install started: version={}", pending->Version);
    return true;
  }
  catch(...)
  {
    LOG_WARN("update install failed: {}", bibstd::util::exception_report());
    return false;
  }
}

} // namespace verselens
