#pragma once

#include "bibqml/util/SettingConversion.hpp"

#include <bibstd/framework/synchronized_executor.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QVariant>

#include <optional>

namespace bibqml
{

///
/// QML setting binding, binds `value` to the setting at a path in both directions. A missing setting is created
/// from the default value, unbound and of its type (boolean, integral, floating point, string or color, kept as
/// "#AARRGGBB"), so QML declares settings of its own. Created by BridgeSettings only, path and default never change:
/// \code
///   readonly property SettingBinding accentColor: BridgeSettings.binding("ui.color.accent", "#1e301e")
///   color: accentColor.value
/// \endcode
/// \see BridgeSettings
///
class SettingBinding final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("SettingBinding is created by BridgeSettings")

  Q_PROPERTY(QString path READ path CONSTANT FINAL)
  Q_PROPERTY(QVariant defaultValue READ defaultValue CONSTANT FINAL)
  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged FINAL)
  Q_PROPERTY(bool bound READ bound CONSTANT FINAL)

  // Variables
  const QString path_;
  const QVariant defaultValue_;
  std::optional<SettingVariantType> setting_;
  bibstd::framework::synchronized_executor executor_;

public: // Structors
  ///
  /// Bind to the setting of the specified path, creating it from the default value if no
  /// setting of that path exists yet. A binding that cannot be established reports the
  /// default value as value and rejects any write.
  ///
  explicit SettingBinding(QString path, QVariant defaultValue, bibstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~SettingBinding() noexcept override;

public: // Accessors
  ///
  /// \return path of the bound setting
  ///
  [[nodiscard]] QString path() const;

  ///
  /// \return default value the setting is created with
  ///
  [[nodiscard]] QVariant defaultValue() const;

  ///
  /// \return current value of the setting, the default value if this binding is not bound
  ///
  [[nodiscard]] QVariant value() const;

  ///
  /// \return true if this binding is bound to a setting, false otherwise
  ///
  [[nodiscard]] bool bound() const;

public: // Setters
  ///
  /// Write the value to the bound setting. The value is validated by the setting, therefore
  /// the value of the setting may differ from the value written.
  ///
  void setValue(const QVariant& value);

signals:
  void valueChanged();

private: // Implementation
  ///
  /// Bind to the setting of the path, creating it if it does not exist yet.
  /// This is called once, while this binding is constructed.
  ///
  void bind();
};

} // namespace bibqml
