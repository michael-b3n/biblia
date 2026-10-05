#include "bibqml/bridge/BridgeBibleRefLookup.hpp"
#include "bibqml/util/ScriptureAccess.hpp"

#include <bibstd/util/format.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/workflow/workflow_bible_ref_lookup.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <QMetaObject>

namespace bibqml
{

///
///
BridgeBibleRefLookup::BridgeBibleRefLookup(
  std::shared_ptr<bibstd::workflow::workflow_bible_ref_lookup> workflowBibleRefLookup,
  std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture,
  const bibstd::util::non_owning_ptr<QObject> parent
)
  : QObject{parent}
  , workflowBibleRefLookup_{std::move(workflowBibleRefLookup)}
  , workflowScripture_{std::move(workflowScripture)}
{
  workflowBibleRefLookup_->connect_queued(
    &bibstd::workflow::workflow_bible_ref_lookup_sigs::ended,
    [this](const bibstd::framework::process_id_type processId)
    {
      QMetaObject::invokeMethod(
        this,
        [this, processId]()
        {
          if(processId_ == processId && running_)
          {
            running_ = false;
            emit runningChanged(running_);
          }
        },
        Qt::QueuedConnection
      );
    },
    executor_
  );
}

///
///
BridgeBibleRefLookup::~BridgeBibleRefLookup() noexcept = default;

///
///
void BridgeBibleRefLookup::lookup(
  const QString& bookId, const int chapterBegin, const int verseBegin, const int chapterEnd, const int verseEnd
)
{
  using params = bibstd::workflow::workflow_scripture::scripture_params::value_type;
  const auto versification = workflowScripture_->versification_or_fallback(params{});

  const auto begin = toReference(versification.get(), bookId, chapterBegin, verseBegin);
  if(!begin)
  {
    return;
  }
  const auto end = toReference(versification.get(), bookId, chapterEnd, verseEnd);
  start({
    bibstd::bible::reference_range{*begin, end.value_or(*begin)}
  });
}

///
///
void BridgeBibleRefLookup::lookupChapter(const QString& bookId, const int chapter)
{
  using params = bibstd::workflow::workflow_scripture::scripture_params::value_type;
  const auto versification = workflowScripture_->versification_or_fallback(params{});
  const auto first = toReference(versification.get(), bookId, chapter, 1);
  if(!first)
  {
    return;
  }
  const auto verseCount = versification.get().verse_count(first->book(), first->chapter());
  const auto last = bibstd::bible::reference::create(first->book(), chapter, verseCount, versification.get());
  start({
    bibstd::bible::reference_range{*first, last.value_or(*first)}
  });
}

///
///
void BridgeBibleRefLookup::disconnect()
{
  executor_.disconnect();
}

///
///
void BridgeBibleRefLookup::start(std::vector<bibstd::bible::reference_range>&& ranges)
{
  const auto params = bibstd::workflow::workflow_bible_ref_lookup::params{{std::move(ranges)}};
  processId_ = params.process_id();
  if(!running_)
  {
    running_ = true;
    emit runningChanged(running_);
  }
  LOG_DEBUG("start bible reference lookup: references=[{}]", bibstd::util::format::join(params->references, ", "));
  workflowBibleRefLookup_->lookup(params);
}

} // namespace bibqml
