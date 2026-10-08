#pragma once

#include <bibstd/framework/synchronized_executor.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <qtmetamacros.h>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>

#include <memory>

namespace bibstd::workflow
{
// Forward declarations
class workflow_script;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// QML bridge for workflow_script.
/// This lists the loaded user scripts and loads them anew, e.g. once they or their settings changed.
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
  bibstd::framework::synchronized_executor executor_;

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
  /// Disconnect all signal connections.
  /// This will stop the frontend backend communication.
  ///
  void disconnect();

private: // Implementation
  void updateScripts();

signals:
  void loadingChanged();
  void scriptsChanged();
};

} // namespace bibqml
