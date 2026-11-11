#pragma once

#include <arcstd/execution/scope.hpp>
#include <arcstd/framework/queued_connections.hpp>
#include <arcstd/util/non_owning_ptr.hpp>

#include <QList>
#include <QObject>
#include <qtmetamacros.h>
#include <QtQml/qqmlregistration.h>
#include <QUrl>

#include <memory>

namespace bibstd::workflow
{
class workflow_scripture;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// QML bridge for workflow_scripture.
/// This tells whether any scripture is loaded, takes scripture files into the scripture folder and reads them anew.
///
class BridgeScripture final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool available MEMBER available_ NOTIFY availableChanged FINAL)
  Q_PROPERTY(bool loaded MEMBER loaded_ NOTIFY loadedChanged FINAL)
  Q_PROPERTY(bool reading MEMBER reading_ NOTIFY readingChanged FINAL)

  // Variables
  const std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture_;
  bool available_{false};
  bool loaded_{false};
  bool reading_{false};
  arcstd::framework::queued_connections connections_;
  arcstd::execution::scope scope_;

public: // Structors
  explicit BridgeScripture(
    std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture,
    arcstd::util::non_owning_ptr<QObject> parent = nullptr
  );
  ~BridgeScripture() noexcept override;

public: // Modifiers
  ///
  /// Take the scripture \p files into the scripture folder and load them. The files are expected as
  /// the URLs a file dialog reports. Only one import runs at a time, a call made while one runs is
  /// ignored, as is one naming no local file.
  ///
  Q_INVOKABLE void importFiles(const QList<QUrl>& files);

  ///
  /// Read the scripture files anew. Like an import, a call made while one reads is ignored.
  ///
  Q_INVOKABLE void reload();

  ///
  /// Disconnect all signal connections.
  /// This will stop the frontend backend communication.
  ///
  void disconnect();

signals:
  void availableChanged();
  void loadedChanged();
  void readingChanged();
  void importEnded(int count);

private: // Implementation
  void updateAvailable();
  void updateLoaded();
  void setReading(bool reading);
};

} // namespace bibqml
