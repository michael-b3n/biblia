#include "bibqml/bridge/BridgeScripture.hpp"
#include "bibqml/util/LocalPath.hpp"

#include <bibstd/execution/launch.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <QMetaObject>

#include <cstddef>
#include <utility>

namespace bibqml
{

///
///
BridgeScripture::BridgeScripture(
  std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture, const bibstd::util::non_owning_ptr<QObject> parent
)
  : QObject{parent}
  , workflowScripture_{std::move(workflowScripture)}
{
  // Scripts add scriptures once they are loaded
  workflowScripture_->connect_queued(
    &bibstd::workflow::workflow_scripture_sigs::scriptures_changed,
    [this]() { QMetaObject::invokeMethod(this, [this]() { updateAvailable(); }, Qt::QueuedConnection); },
    connections_
  );
  // After connecting, so no change is missed
  available_ = workflowScripture_->scripture_count() > 0;
}

///
///
BridgeScripture::~BridgeScripture() noexcept = default;

///
///
void BridgeScripture::importFiles(const QList<QUrl>& files)
{
  if(importRunning_)
  {
    return;
  }
  auto sources = LocalPath::toLocalPaths(files);
  if(sources.empty())
  {
    return;
  }
  LOG_DEBUG("start scripture import: files={}", sources.size());
  const auto params = bibstd::workflow::workflow_scripture::import_params{{.sources = std::move(sources)}};
  importRunning_ = true;
  emit importRunningChanged();
  auto finish = [this](const std::size_t imported)
  {
    if(importRunning_)
    {
      importRunning_ = false;
      emit importRunningChanged();
    }
    updateAvailable();
    emit importEnded(static_cast<int>(imported));
  };
  auto import = [this, params, finish = std::move(finish)]()
  {
    const auto imported = workflowScripture_->import_scriptures(params);
    QMetaObject::invokeMethod(this, [finish, imported]() { finish(imported); }, Qt::QueuedConnection);
  };
  bibstd::execution::launch(std::move(import), scope_);
}

///
///
void BridgeScripture::disconnect()
{
  connections_.disconnect();
  scope_.stop();
}

///
///
void BridgeScripture::updateAvailable()
{
  if(const auto available = workflowScripture_->scripture_count() > 0; available_ != available)
  {
    available_ = available;
    emit availableChanged();
  }
}

} // namespace bibqml
