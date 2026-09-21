#pragma once

#include "bibqml/util/SettingConversion.hpp"

#include <bibstd/signal/synchronized_executor.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QVariant>

#include <optional>

namespace bibqml
{

///
/// QML setting binding.
/// This element binds a QML property to a setting of the settings workflow. If no setting
/// exists for the given path, a new one is created from the given default value. This allows
/// the QML layer to declare its own settings, e.g. style colors, without the backend knowing
/// about them. Settings declared this way are persisted like any other setting.
///
/// A binding is created by the settings registry, it cannot be declared in QML. This keeps
/// path and default value constant for the lifetime of the binding, only `value` changes at
/// runtime, in both directions.
///
/// Usage:
/// \code
///   readonly property SettingBinding accentColor: BridgeSettings.binding("ui.color.accent", "#1e301e")
///   ...
///   color: accentColor.value
/// \endcode
///
/// A setting that is created by this binding is created with the value type of the default value.
/// Supported are boolean, integral, floating point, string and color values. Colors are stored
/// as "#AARRGGBB" strings. Settings declared from QML are unbound, they have no validator.
/// Reading and writing `value` is defined by the value type of the bound setting, therefore a
/// binding can also be attached to a setting of any value type the backend declared.
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
  bibstd::signal::synchronized_executor executor_;

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
