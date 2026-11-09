#include "bibqml/bridge/BridgeScripture.hpp"
#include "arca_qml/util/LocalPath.hpp"

#include <arca/execution/launch.hpp>
#include <arca/util/log.hpp>
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
    [this]()
    {
      QMetaObject::invokeMethod(
        this,
        [this]()
        {
          updateAvailable();
          updateLoaded();
        },
        Qt::QueuedConnection
      );
    },
    connections_
  );
  // After connecting, so no change is missed
  available_ = workflowScripture_->scripture_count() > 0;
  loaded_ = workflowScripture_->loaded();
}

///
///
BridgeScripture::~BridgeScripture() noexcept = default;

///
///
void BridgeScripture::importFiles(const QList<QUrl>& files)
{
  if(reading_)
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
  setReading(true);
  auto finish = [this](const std::size_t imported)
  {
    setReading(false);
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
void BridgeScripture::reload()
{
  if(reading_)
  {
    return;
  }
  setReading(true);
  auto read = [this]()
  {
    workflowScripture_->reload_scriptures();
    QMetaObject::invokeMethod(this, [this]() { setReading(false); }, Qt::QueuedConnection);
  };
  bibstd::execution::launch(std::move(read), scope_);
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

///
///
void BridgeScripture::updateLoaded()
{
  if(const auto loaded = workflowScripture_->loaded(); loaded_ != loaded)
  {
    loaded_ = loaded;
    emit loadedChanged();
  }
}

///
///
void BridgeScripture::setReading(const bool reading)
{
  if(reading_ != reading)
  {
    reading_ = reading;
    emit readingChanged();
  }
}

} // namespace bibqml
