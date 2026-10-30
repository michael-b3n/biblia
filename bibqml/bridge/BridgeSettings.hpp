#pragma once

#include "bibqml/bridge/SettingBinding.hpp"

#include <bibstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <QString>
#include <QtQml/qqmlengine.h>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QVariant>

#include <map>
#include <memory>

// Forward declarations
namespace bibstd::workflow
{
class workflow_settings;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// QML settings registry, creates the setting bindings of the QML layer. Owned by the application, it is created
/// before the QML engine loads and outlives it.
/// \see SettingBinding
///
class BridgeSettings final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  // Variables
  const std::shared_ptr<bibstd::workflow::workflow_settings> workflowSettings_;
  std::map<QString, std::unique_ptr<SettingBinding>> bindings_;

public: // Static interface
  ///
  /// Access the instance the QML layer operates on.
  /// \return the instance, nullptr if no instance exists
  ///
  [[nodiscard]] static bibstd::util::non_owning_ptr<BridgeSettings> instance();

  ///
  /// QML singleton factory. The returned instance stays owned by the application.
  /// \return the instance, nullptr if no instance exists
  ///
  static BridgeSettings* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

public: // Structors
  explicit BridgeSettings(
    std::shared_ptr<bibstd::workflow::workflow_settings> workflowSettings,
    bibstd::util::non_owning_ptr<QObject> parent = nullptr
  );
  ~BridgeSettings() noexcept override;

public: // Accessors
  ///
  /// Access the settings workflow all setting bindings operate on.
  /// \return settings workflow
  ///
  [[nodiscard]] const std::shared_ptr<bibstd::workflow::workflow_settings>& workflowSettings() const;

  ///
  /// Check if a setting with the specified path exists.
  /// \return true if the setting exists, false otherwise
  ///
  Q_INVOKABLE bool contains(const QString& path) const;

public: // Modifiers
  ///
  /// Access the binding of \p path, creating it and the setting on first access. All callers share one binding,
  /// the default value of the first call wins.
  /// \return setting binding owned by this registry, nullptr if none could be created
  ///
  Q_INVOKABLE SettingBinding* binding(const QString& path, const QVariant& defaultValue);
};

} // namespace bibqml
