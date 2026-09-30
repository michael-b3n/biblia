#include "bibqml/bridge/BridgeScript.hpp"

#include <bibstd/util/log.hpp>
#include <bibstd/workflow/workflow_script.hpp>

#include <QMetaObject>
#include <QStringList>
#include <QVariantMap>

#include <ranges>
#include <utility>

namespace bibqml
{

///
///
BridgeScript::BridgeScript(
  std::shared_ptr<bibstd::workflow::workflow_script> workflowScript, const bibstd::util::non_owning_ptr<QObject> parent
)
  : QObject{parent}
  , workflowScript_{std::move(workflowScript)}
{
  workflowScript_->connect_queued(
    &bibstd::workflow::workflow_script_sigs::scripts_loaded,
    [this]()
    {
      QMetaObject::invokeMethod(
        this,
        [this]()
        {
          updateScripts();
          if(loading_)
          {
            loading_ = false;
            emit loadingChanged();
          }
        },
        Qt::QueuedConnection
      );
    },
    executor_
  );
  updateScripts();
}

///
///
BridgeScript::~BridgeScript() noexcept = default;

///
///
void BridgeScript::loadScripts()
{
  if(loading_)
  {
    return;
  }
  loading_ = true;
  emit loadingChanged();
  LOG_DEBUG("load scripts anew");
  workflowScript_->load_scripts();
}

///
///
void BridgeScript::disconnect()
{
  executor_.disconnect();
}

///
///
void BridgeScript::updateScripts()
{
  const auto toEntry = [](const auto& script)
  {
    const auto functions = script.second.functions |
                           std::views::transform([](const auto& id) { return QString::fromStdString(id.string()); }) |
                           std::ranges::to<QStringList>();
    auto entry = QVariantMap{};
    entry.insert(QStringLiteral("id"), QString::fromStdString(script.first.string()));
    entry.insert(QStringLiteral("name"), QString::fromStdString(script.second.name));
    entry.insert(QStringLiteral("functions"), functions);
    return QVariant{entry};
  };
  const auto scripts = workflowScript_->scripts();
  scripts_ = scripts | std::views::transform(toEntry) | std::ranges::to<QVariantList>();
  emit scriptsChanged();
}

} // namespace bibqml
