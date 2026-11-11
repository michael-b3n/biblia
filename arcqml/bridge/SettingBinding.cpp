#include "arcqml/bridge/SettingBinding.hpp"
#include "arcqml/bridge/BridgeSettings.hpp"
#include "arcqml/util/SettingConversion.hpp"

#include <arcstd/framework/setting_validator.hpp>
#include <arcstd/util/exception.hpp>
#include <arcstd/util/log.hpp>
#include <arcstd/util/visit_helper.hpp>

#include <QMetaObject>

#include <format>
#include <memory>
#include <ranges>
#include <utility>
#include <variant>
#include <vector>

namespace bibqml
{
namespace
{

///
/// \throws util::exception if \p value cannot be converted to the type of \p defaultValue
/// \return \p value as setting value, whose type \p T is the one of \p defaultValue
///
template<typename T>
[[nodiscard]] auto toValidatorValue(const QVariant& value, const QVariant& defaultValue) -> T
{
  auto converted = tryNormalizeQVariant(value);
  if(!converted.convert(tryNormalizeQVariant(defaultValue).metaType()))
  {
    throw bibstd::util::exception{
      std::format(R"(validator value does not match the default value: value="{}")", value.toString().toStdString())
    };
  }
  return std::get<T>(*toDefaultSettingValue(converted));
}

} // namespace

///
///
SettingBinding::SettingBinding(QString path, QVariant defaultValue, const bibstd::util::non_owning_ptr<QObject> parent)
  : SettingBinding{std::move(path), std::move(defaultValue), Validator{}, parent}
{
}

///
///
SettingBinding::SettingBinding(
  QString path, QVariant defaultValue, QVariantList available, const bibstd::util::non_owning_ptr<QObject> parent
)
  : SettingBinding{std::move(path), std::move(defaultValue), Validator{std::move(available)}, parent}
{
}

///
///
SettingBinding::SettingBinding(
  QString path, QVariant defaultValue, QVariant minimum, QVariant maximum, const bibstd::util::non_owning_ptr<QObject> parent
)
  : SettingBinding{
      std::move(path),
      std::move(defaultValue),
      Validator{ValueRange{.minimum = std::move(minimum), .maximum = std::move(maximum)}},
      parent
    }
{
}

///
///
SettingBinding::~SettingBinding() noexcept = default;

///
///
QString SettingBinding::path() const
{
  return path_;
}

///
///
QVariant SettingBinding::defaultValue() const
{
  return defaultValue_;
}

///
///
QVariant SettingBinding::value() const
{
  return setting_ ? toQmlValue(*setting_) : defaultValue_;
}

///
///
bool SettingBinding::bound() const
{
  return setting_.has_value();
}

///
///
void SettingBinding::setValue(const QVariant& value)
{
  if(!setting_)
  {
    LOG_ERROR("write setting value failed: binding is not bound: path=\"{}\"", path_.toStdString());
    return;
  }
  try
  {
    // The conversion of the value is defined by the value type of the setting.
    const auto isValueSet = setQmlValue(*setting_, value);
    if(!isValueSet)
    {
      // The value was rejected. Notify the QML layer such that it can reset to the current value.
      // If the setting changed its value, a second notification follows.
      emit valueChanged();
    }
  }
  catch(...)
  {
    LOG_ERROR("exception writing setting value: {}", bibstd::util::exception_report());
    emit valueChanged();
  }
}

///
///
SettingBinding::SettingBinding(
  QString path, QVariant defaultValue, Validator validator, const bibstd::util::non_owning_ptr<QObject> parent
)
  : QObject{parent}
  , path_{std::move(path)}
  , defaultValue_{std::move(defaultValue)}
  , validator_{std::move(validator)}
{
  bind();
}

///
///
void SettingBinding::bind()
{
  // Since path and default value are constant, this binds exactly once.
  if(path_.isEmpty())
  {
    LOG_ERROR("bind setting failed: no path provided");
    return;
  }
  const auto* const registry = BridgeSettings::instance();
  if(registry == nullptr)
  {
    LOG_ERROR("bind setting failed: settings registry does not exist: path=\"{}\"", path_.toStdString());
    return;
  }
  const auto& workflowSettings = registry->workflowSettings();
  const auto defaultSettingValue = toDefaultSettingValue(defaultValue_);
  if(!defaultSettingValue)
  {
    LOG_ERROR("bind setting failed: unsupported default value type: path=\"{}\"", path_.toStdString());
    return;
  }

  try
  {
    setting_ = std::visit(
      [&]<typename T>(const T& v) -> SettingVariantType
      { return workflowSettings->type_erased_setting(path_.toStdString(), v, settingValidator<T>()); },
      *defaultSettingValue
    );
  }
  catch(...)
  {
    // Without a setting the default value stays the value of this binding.
    LOG_ERROR("bind setting failed: {}", bibstd::util::exception_report());
    return;
  }

  std::visit(
    [this](const auto setting)
    {
      setting->signal_adapter.connect_queued(
        &bibstd::framework::setting_signals::value_changed,
        [this]() { QMetaObject::invokeMethod(this, [this]() { emit valueChanged(); }, Qt::QueuedConnection); },
        connections_
      );
    },
    *setting_
  );
  LOG_DEBUG("bind setting: path=\"{}\"", path_.toStdString());
}

///
///
template<typename T>
auto SettingBinding::settingValidator() const -> bibstd::framework::setting_validator<T>
{
  using list_type = bibstd::framework::setting_validator_list<T>;
  using range_type = bibstd::framework::setting_validator_range<T>;
  using return_type = bibstd::framework::setting_validator<T>;
  const auto toValue = [this](const QVariant& value) { return toValidatorValue<T>(value, defaultValue_); };
  return bibstd::util::visit_lambdas(
    validator_,
    [](std::monostate) -> return_type { return std::make_shared<bibstd::framework::setting_validator_unbound>(); },
    [&](const QVariantList& available) -> return_type
    {
      if constexpr(list_type::is_plain_type)
      {
        return std::make_shared<list_type>(available | std::views::transform(toValue) | std::ranges::to<std::vector>());
      }
      else
      {
        throw bibstd::util::exception{"list validator needs a plain setting"};
      }
    },
    [&](const ValueRange& range) -> return_type
    {
      if constexpr(range_type::is_plain_type && bibstd::framework::detail::setting_validator_range_value_type<T>)
      {
        return std::make_shared<range_type>(toValue(range.minimum), toValue(range.maximum));
      }
      else
      {
        throw bibstd::util::exception{"range validator needs a number setting"};
      }
    }
  );
}

} // namespace bibqml
