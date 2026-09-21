#pragma once

#include <bibstd/workflow/workflow_settings.hpp>

#include <QString>
#include <QVariant>

#include <optional>

namespace bibqml
{

///
/// Variant holding a non owning pointer to a type erased setting of any supported value type.
///
using SettingVariantType = bibstd::workflow::workflow_settings::setting_type_erased_non_owning_ptr_variant_type;

///
/// Deduce the default value of a new setting from a QML provided value. The alternative of the
/// returned variant defines the value type a setting declared in QML is created with. Reading
/// and writing an existing setting is defined by the value type of that setting.
/// Supported are boolean, integral, floating point, string and color values.
/// Colors are converted to "#AARRGGBB" strings.
/// \return default setting value, std::nullopt if the value type is not supported
///
[[nodiscard]] auto toDefaultSettingValue(const QVariant& value)
  -> std::optional<bibstd::framework::setting_type_erased_variant>;

///
/// Convert the value of a type erased setting to a QML value.
/// The conversion is defined by the value type of the setting.
/// \return QML value, an invalid value if the setting holds no value
///
[[nodiscard]] auto toQmlValue(const SettingVariantType& setting) -> QVariant;

///
/// Convert a QML value to the value type of a type erased setting and write it to the setting.
/// The value is validated by the setting, therefore the value of the setting may differ from
/// the value written.
/// \throws util::exception if the QML value does not match the value type of the setting
/// \return true if the value was set, false otherwise
///
[[nodiscard]] auto setQmlValue(const SettingVariantType& setting, const QVariant& value) -> bool;

///
/// Postfix a type erased setting is displayed behind its value with, e.g. the unit it is
/// measured in. A setting naming its own is displayed with that one. A duration names none:
/// it is handed over counting the period it is stored in, \see toQmlValue, so the unit of
/// that period names it.
/// \return postfix to display behind the value, empty if the setting has none
///
[[nodiscard]] auto toQmlPostfix(const SettingVariantType& setting) -> QString;

///
/// Access all values the list validator of a type erased setting provides.
/// \return QML values of all available values, empty if the setting has no list validator
///
[[nodiscard]] auto toQmlValidatorValues(const SettingVariantType& setting) -> QList<QVariant>;

} // namespace bibqml
