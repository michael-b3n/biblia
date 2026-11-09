#include "arca_qml/bridge/BridgeSettings.hpp"
#include "arca_qml/bridge/SettingBinding.hpp"

#include <arca/util/log.hpp>
#include <arca/workflow/workflow_settings.hpp>

#include <memory>
#include <utility>

namespace bibqml
{
namespace
{

// Instance the QML layer operates on. It is registered by the constructor of the
// application owned instance and reset when that instance is destroyed.
bibstd::util::non_owning_ptr<BridgeSettings> instance_{nullptr};

} // anonymous namespace

///
///
bibstd::util::non_owning_ptr<BridgeSettings> BridgeSettings::instance()
{
  return instance_;
}

///
///
BridgeSettings* BridgeSettings::create([[maybe_unused]] QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
  if(instance_ == nullptr)
  {
    LOG_ERROR("settings registry does not exist: settings cannot be bound from qml");
    return nullptr;
  }
  // The instance is owned by the application, the QML engine must not delete it.
  if(jsEngine != nullptr)
  {
    QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
  }
  return instance_;
}

///
///
BridgeSettings::BridgeSettings(
  std::shared_ptr<bibstd::workflow::workflow_settings> workflowSettings, const bibstd::util::non_owning_ptr<QObject> parent
)
  : QObject{parent}
  , workflowSettings_{std::move(workflowSettings)}
{
  if(instance_ != nullptr)
  {
    LOG_WARN("settings registry already exists: qml will operate on the first instance");
    return;
  }
  instance_ = this;
}

///
///
BridgeSettings::~BridgeSettings() noexcept
{
  if(instance_ == this)
  {
    instance_ = nullptr;
  }
}

///
///
const std::shared_ptr<bibstd::workflow::workflow_settings>& BridgeSettings::workflowSettings() const
{
  return workflowSettings_;
}

///
///
bool BridgeSettings::contains(const QString& path) const
{
  return workflowSettings_ && workflowSettings_->type_erased_setting(path.toStdString()).has_value();
}

///
///
SettingBinding* BridgeSettings::binding(const QString& path, const QVariant& defaultValue)
{
  return bindingOf(path, defaultValue, [&]() { return std::make_unique<SettingBinding>(path, defaultValue); });
}

///
///
SettingBinding* BridgeSettings::binding(const QString& path, const QVariant& defaultValue, const QVariantList& available)
{
  return bindingOf(path, defaultValue, [&]() { return std::make_unique<SettingBinding>(path, defaultValue, available); });
}

///
///
SettingBinding* BridgeSettings::binding(
  const QString& path, const QVariant& defaultValue, const QVariant& minimum, const QVariant& maximum
)
{
  return bindingOf(
    path, defaultValue, [&]() { return std::make_unique<SettingBinding>(path, defaultValue, minimum, maximum); }
  );
}

///
///
SettingBinding* BridgeSettings::bindingOf(
  const QString& path, const QVariant& defaultValue, const std::function<std::unique_ptr<SettingBinding>()>& create
)
{
  if(path.isEmpty())
  {
    LOG_ERROR("access setting binding failed: no path provided");
    return nullptr;
  }
  if(const auto it = bindings_.find(path); it != std::cend(bindings_))
  {
    if(it->second->defaultValue() != defaultValue)
    {
      LOG_WARN("setting binding exists with a different default value: path=\"{}\"", path.toStdString());
    }
    return it->second.get();
  }
  auto binding = create();
  // The binding is owned by this registry, the QML engine must not delete it.
  QJSEngine::setObjectOwnership(binding.get(), QJSEngine::CppOwnership);
  return bindings_.emplace(path, std::move(binding)).first->second.get();
}

} // namespace bibqml
