#include "bibqml/bridge/BridgeBibleRefCopy.hpp"
#include "bibqml/util/ScriptureAccess.hpp"

#include <arcstd/execution/launch.hpp>
#include <arcstd/util/log.hpp>
#include <bibstd/workflow/workflow_bible_ref_copy.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <QMetaObject>

#include <utility>

namespace bibqml
{

///
///
BridgeBibleRefCopy::BridgeBibleRefCopy(
  std::shared_ptr<bibstd::workflow::workflow_bible_ref_copy> workflowBibleRefCopy,
  std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture,
  const arcstd::util::non_owning_ptr<QObject> parent
)
  : QObject{parent}
  , workflowBibleRefCopy_{std::move(workflowBibleRefCopy)}
  , workflowScripture_{std::move(workflowScripture)}
{
}

///
///
BridgeBibleRefCopy::~BridgeBibleRefCopy() noexcept = default;

///
///
void BridgeBibleRefCopy::copy(
  const QString& bookId, const int chapterBegin, const int verseBegin, const int chapterEnd, const int verseEnd
)
{
  if(running_)
  {
    return;
  }
  using scripture_params = bibstd::workflow::workflow_scripture::scripture_params::value_type;
  const auto versification = workflowScripture_->versification_or_fallback(scripture_params{});
  const auto range = toReferenceRange(versification.get(), bookId, chapterBegin, verseBegin, chapterEnd, verseEnd);
  if(!range)
  {
    return;
  }
  const auto params = bibstd::workflow::workflow_bible_ref_copy::params{{.range = *range}};
  running_ = true;
  emit runningChanged(running_);
  LOG_DEBUG("start bible reference copy: reference=\"{}\"", params->range);
  auto finish = [this](const bool copied)
  {
    running_ = false;
    emit runningChanged(running_);
    if(copied)
    {
      emit this->copied();
    }
  };
  auto copy = [this, params, finish = std::move(finish)]()
  {
    const auto copied = workflowBibleRefCopy_->copy(params);
    QMetaObject::invokeMethod(this, [finish, copied]() { finish(copied); }, Qt::QueuedConnection);
  };
  arcstd::execution::launch(std::move(copy), scope_);
}

///
///
void BridgeBibleRefCopy::disconnect()
{
  scope_.stop();
}

} // namespace bibqml
