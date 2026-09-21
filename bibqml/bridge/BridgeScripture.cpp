#include "bibqml/bridge/BridgeScripture.hpp"

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
  , available_{workflowScripture_->scripture_count() > 0}
{
  workflowScripture_->connect_queued(
    &bibstd::workflow::workflow_scripture_sigs::import_ended,
    [this](const bibstd::framework::process_id_type processId, const std::size_t imported)
    {
      QMetaObject::invokeMethod(
        this,
        [this, processId, imported]()
        {
          if(processId_ != processId)
          {
            return;
          }
          if(importRunning_)
          {
            importRunning_ = false;
            emit importRunningChanged();
          }
          if(const auto available = workflowScripture_->scripture_count() > 0; available_ != available)
          {
            available_ = available;
            emit availableChanged();
          }
          emit importEnded(static_cast<int>(imported));
        },
        Qt::QueuedConnection
      );
    },
    executor_
  );
}

///
///
BridgeScripture::~BridgeScripture() noexcept = default;

///
///
void BridgeScripture::importFolder(const QUrl& folder)
{
  if(importRunning_)
  {
    return;
  }
  // Empty for anything that does not name a folder of this machine, which is nothing to import from
  const auto localFolder = folder.toLocalFile();
  if(localFolder.isEmpty())
  {
    LOG_WARN("scripture import failed: no local folder provided: url=\"{}\"", folder.toString().toStdString());
    return;
  }
  const auto params = bibstd::workflow::workflow_scripture::import_params{{.folder = localFolder.toStdString()}};
  processId_ = params.process_id();
  importRunning_ = true;
  emit importRunningChanged();
  LOG_DEBUG("start scripture import: folder=\"{}\"", localFolder.toStdString());
  workflowScripture_->import_scriptures(params);
}

///
///
void BridgeScripture::disconnect()
{
  executor_.disconnect();
}

} // namespace bibqml
