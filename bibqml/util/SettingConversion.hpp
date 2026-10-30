#pragma once

#include <bibstd/framework/setting_validator.hpp>
#include <bibstd/workflow/workflow_settings.hpp>

#include <QString>
#include <QVariant>

#include <optional>
#include <type_traits>
#include <vector>

namespace bibqml
{

///
/// Variant holding a non owning pointer to a type erased setting of any supported value type.
///
using SettingVariantType = bibstd::workflow::workflow_settings::setting_type_erased_non_owning_ptr_variant_type;

///
/// Type trait that removes the optional or vector from a setting value type.
/// Not meta::remove_wrapper, which also takes std::string and durations apart.
///
template<typename T>
struct SettingValueType final
{
  using type = T;
};
template<typename T>
struct SettingValueType<std::optional<T>> final
{
  using type = T;
};
template<typename T>
struct SettingValueType<std::vector<T>> final
{
  using type = T;
};

///
/// Value type of the setting a pointer of type \p T points to, without its optional or vector.
///
template<typename T>
using SettingRawValueType = typename SettingValueType<typename std::remove_pointer_t<T>::value_type>::type;

///
/// False for every \p T, for the static_assert of an if constexpr branch no type should reach.
///
template<typename T>
inline constexpr bool alwaysFalse = false;

///
/// Shared pointers to the validators a type erased setting holds, one per validator kind.
///
template<typename T>
using ValidatorRangeSptrType = bibstd::framework::setting_validator_range_type_erased<T>::sptr_type;
template<typename T>
using ValidatorListSptrType = bibstd::framework::setting_validator_list_type_erased<T>::sptr_type;
using ValidatorUnboundSptrType = bibstd::framework::setting_validator_unbound::sptr_type;

///
/// \return the value of a QJSValue QML wraps \p variant in, \p variant itself if it is none
///
[[nodiscard]] auto tryNormalizeQVariant(const QVariant& variant) -> QVariant;

///
/// Deduce the default value of a setting declared in QML from \p value, its alternative is the type of the setting:
/// boolean, integral, floating point, string, or a color as "#AARRGGBB" string.
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
/// Write a QML value to a type erased setting, which validates it, so its value may differ from the one written.
/// \throws util::exception if the QML value does not match the value type of the setting
/// \return true if the value was set, false otherwise
///
[[nodiscard]] auto setQmlValue(const SettingVariantType& setting, const QVariant& value) -> bool;

///
/// Postfix displayed behind the value of a type erased setting, e.g. its unit. A duration without one of its own
/// takes the unit of its period, \see toQmlValue.
/// \return postfix to display behind the value, empty if the setting has none
///
[[nodiscard]] auto toQmlPostfix(const SettingVariantType& setting) -> QString;

///
/// Access all values the list validator of a type erased setting provides.
/// \return QML values of all available values, empty if the setting has no list validator
///
[[nodiscard]] auto toQmlValidatorValues(const SettingVariantType& setting) -> QList<QVariant>;

} // namespace bibqml
