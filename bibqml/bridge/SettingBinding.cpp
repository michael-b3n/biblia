#include "bibqml/bridge/SettingBinding.hpp"
#include "bibqml/bridge/BridgeSettings.hpp"
#include "bibqml/util/SettingConversion.hpp"

#include <bibstd/util/exception.hpp>
#include <bibstd/util/log.hpp>

#include <QMetaObject>

#include <utility>
#include <variant>

namespace bibqml
{

///
///
SettingBinding::SettingBinding(QString path, QVariant defaultValue, const bibstd::util::non_owning_ptr<QObject> parent)
  : QObject{parent}
  , path_{std::move(path)}
  , defaultValue_{std::move(defaultValue)}
{
  bind();
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
      [&](const auto& v) { return workflowSettings->type_erased_setting(path_.toStdString(), v); }, *defaultSettingValue
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
        executor_
      );
    },
    *setting_
  );
  LOG_DEBUG("bind setting: path=\"{}\"", path_.toStdString());
}

} // namespace bibqml
