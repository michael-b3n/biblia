#pragma once

#include <bibstd/framework/process_params.hpp>
#include <bibstd/signal/synchronized_executor.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <qtmetamacros.h>
#include <QtQml/qqmlregistration.h>
#include <QUrl>

#include <memory>

namespace bibstd::workflow
{
// Forward declarations
class workflow_scripture;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// QML bridge for workflow_scripture.
/// This tells whether any scripture is loaded and takes scripture files into the scripture folder.
///
class BridgeScripture final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool available MEMBER available_ NOTIFY availableChanged FINAL)
  Q_PROPERTY(bool importRunning MEMBER importRunning_ NOTIFY importRunningChanged FINAL)

  // Variables
  const std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture_;
  bibstd::framework::process_id_type processId_;
  bool available_{false};
  bool importRunning_{false};
  bibstd::signal::synchronized_executor executor_;

public: // Structors
  explicit BridgeScripture(
    std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture,
    bibstd::util::non_owning_ptr<QObject> parent = nullptr
  );
  ~BridgeScripture() noexcept override;

public: // Modifiers
  ///
  /// Take the scripture files of \p folder into the scripture folder and load them. The folder is
  /// expected as the URL a file dialog reports. Only one import runs at a time, a call made while
  /// one runs is ignored.
  ///
  Q_INVOKABLE void importFolder(const QUrl& folder);

  ///
  /// Disconnect all signal connections.
  /// This will stop the frontend backend communication.
  ///
  void disconnect();

signals:
  void availableChanged();
  void importRunningChanged();
  void importEnded(int count);
};

} // namespace bibqml
