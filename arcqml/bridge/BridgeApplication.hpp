#pragma once

#include <arcstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace arcqml
{

///
/// QML bridge for application wide requests.
/// This forwards requests that reach the application outside of the QML layer, e.g. from the
/// system tray, to the QML layer. A request may be made from any thread.
///
class BridgeApplication : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("BridgeApplication is created by the application")

  Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged FINAL)
  Q_PROPERTY(UpdateCheck updateCheck READ updateCheck NOTIFY updateCheckChanged FINAL)

public: // Typedefs
  ///
  /// State of the update check. It stays unavailable without an updater.
  ///
  enum UpdateCheck
  {
    UpdateCheckUnavailable,
    UpdateCheckIdle,
    UpdateCheckRunning,
    UpdateCheckFinished,
    UpdateCheckFailed
  };
  Q_ENUM(UpdateCheck)

private: // Variables
  bool updateAvailable_{false};
  UpdateCheck updateCheck_{UpdateCheckUnavailable};

public: // Structors
  explicit BridgeApplication(arcstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~BridgeApplication() noexcept override;

public: // Accessors
  ///
  /// \return true if an update is downloaded and ready to be installed
  ///
  [[nodiscard]] bool updateAvailable() const;

  ///
  /// \return state of the update check
  ///
  [[nodiscard]] UpdateCheck updateCheck() const;

public: // Modifiers
  ///
  /// Request the QML layer to show the main window.
  ///
  void requestShowWindow();

  ///
  /// Tell the QML layer that an update is downloaded and ready to be installed.
  ///
  void notifyUpdateAvailable();

  ///
  /// Tell the QML layer the state of the update check.
  ///
  void notifyUpdateCheck(UpdateCheck state);

  ///
  /// Request the application to check for an update right away.
  ///
  Q_INVOKABLE void requestUpdateCheck();

  ///
  /// Request the application to install the available update and restart.
  ///
  Q_INVOKABLE void requestUpdate();

signals:
  void showWindowRequested();
  void updateAvailableChanged();
  void updateCheckChanged();
  void updateCheckRequested();
  void updateRequested();
};

} // namespace arcqml
