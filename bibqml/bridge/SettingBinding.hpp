#pragma once

#include "bibqml/util/SettingConversion.hpp"

#include <arca/framework/queued_connections.hpp>
#include <arca/util/non_owning_ptr.hpp>

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QVariant>
#include <QVariantList>

#include <optional>
#include <variant>

namespace bibqml
{

///
/// QML setting binding, binds `value` to the setting at a path in both directions. A missing setting is created
/// from the default value, so QML declares settings of its own. Created by BridgeSettings only:
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

  // Typedefs
  struct ValueRange final
  {
    QVariant minimum;
    QVariant maximum;
  };
  using Validator = std::variant<std::monostate, QVariantList, ValueRange>;

  // Variables
  const QString path_;
  const QVariant defaultValue_;
  const Validator validator_;
  std::optional<SettingVariantType> setting_;
  bibstd::framework::queued_connections connections_;

public: // Structors
  ///
  /// Bind to the setting of the specified path, creating it from the default value if no
  /// setting of that path exists yet. A binding that cannot be established reports the
  /// default value as value and rejects any write.
  ///
  explicit SettingBinding(QString path, QVariant defaultValue, bibstd::util::non_owning_ptr<QObject> parent = nullptr);
  explicit SettingBinding(
    QString path, QVariant defaultValue, QVariantList available, bibstd::util::non_owning_ptr<QObject> parent = nullptr
  );
  explicit SettingBinding(
    QString path,
    QVariant defaultValue,
    QVariant minimum,
    QVariant maximum,
    bibstd::util::non_owning_ptr<QObject> parent = nullptr
  );
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

private: // Structors
  SettingBinding(QString path, QVariant defaultValue, Validator validator, bibstd::util::non_owning_ptr<QObject> parent);

private: // Implementation
  ///
  /// Bind to the setting of the path, creating it if it does not exist yet.
  /// This is called once, while this binding is constructed.
  ///
  void bind();

  ///
  /// \throws util::exception if a value of this validator does not fit a setting of type \p T
  /// \return the validator of a new setting of type \p T
  ///
  template<typename T>
  [[nodiscard]] auto settingValidator() const -> bibstd::framework::setting_validator<T>;
};

} // namespace bibqml
