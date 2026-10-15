#pragma once

#include <bibstd/execution/scope.hpp>
#include <bibstd/framework/queued_connections.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <QList>
#include <QObject>
#include <qtmetamacros.h>
#include <QtQml/qqmlregistration.h>
#include <QUrl>
#include <QVariantList>

#include <filesystem>
#include <memory>
#include <vector>

namespace bibstd::workflow
{
// Forward declarations
class workflow_script;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// QML bridge for workflow_script.
/// This lists the loaded user scripts, imports script files and loads the scripts anew.
///
class BridgeScript final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool loading MEMBER loading_ NOTIFY loadingChanged FINAL)
  Q_PROPERTY(QVariantList scripts MEMBER scripts_ NOTIFY scriptsChanged FINAL)

  // Variables
  const std::shared_ptr<bibstd::workflow::workflow_script> workflowScript_;
  bool loading_{false};
  QVariantList scripts_;
  bibstd::framework::queued_connections connections_;
  bibstd::execution::scope scope_;

public: // Structors
  explicit BridgeScript(
    std::shared_ptr<bibstd::workflow::workflow_script> workflowScript, bibstd::util::non_owning_ptr<QObject> parent = nullptr
  );
  ~BridgeScript() noexcept override;

public: // Modifiers
  ///
  /// Load the user scripts anew, \see workflow_script::load_scripts. A call made while they load is ignored.
  ///
  Q_INVOKABLE void loadScripts();

  ///
  /// Copy the script \p files into the script folder and load the scripts anew, \see workflow_script::import_scripts.
  /// The files are expected as the URLs a file dialog reports. A call made while the scripts load is ignored.
  ///
  Q_INVOKABLE void importFiles(const QList<QUrl>& files);

  ///
  /// Disconnect all signal connections.
  /// This will stop the frontend backend communication.
  ///
  void disconnect();

private: // Implementation
  void load(std::vector<std::filesystem::path> files);
  void updateScripts();

signals:
  void loadingChanged();
  void scriptsChanged();
};

} // namespace bibqml
