#include "bibqml/model/ScriptureListModel.hpp"
#include "bibqml/util/ScriptureAccess.hpp"

#include <bibstd/util/enum.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/util/ranges.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <algorithm>
#include <limits>
#include <tuple>

namespace bibqml
{
namespace detail
{

///
/// Maximum number of entries the model can hold, limited by the row index type of QAbstractListModel.
///
constexpr auto maxEntries = static_cast<std::size_t>(std::numeric_limits<int>::max());

///
/// Verses loaded on either side of the reference the model is reset with, so the reference is
/// read with what leads up to it instead of from the top of the view.
///
constexpr auto contextSize = 10;

} // namespace detail

///
///
ScriptureListModel::ScriptureListModel(
  std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture, const bibstd::util::non_owning_ptr<QObject> parent
)
  : QAbstractListModel{parent}
  , workflowScripture_{std::move(workflowScripture)}
{
  using ScriptureNameSetting = std::remove_pointer_t<decltype(workflowScripture_->settings().scripture_name)>;
  const bibstd::util::non_owning_ptr<ScriptureNameSetting> scriptureNameSetting = workflowScripture_->settings().scripture_name;

  scriptureNameSetting->connect_queued(
    &ScriptureNameSetting::signals_type::value_changed,
    [this]() { QMetaObject::invokeMethod(this, [this]() { refresh(); }, Qt::QueuedConnection); },
    executor_
  );
  workflowScripture_->connect_queued(
    &bibstd::workflow::workflow_scripture_sigs::scriptures_changed,
    [this]() { QMetaObject::invokeMethod(this, [this]() { refresh(); }, Qt::QueuedConnection); },
    executor_
  );

  executor_.connect(
    entryRequested_,
    [this](const bibstd::bible::reference& ref)
    {
      QMetaObject::invokeMethod(
        this,
        [this, ref, verseText = fetchPassage(ref), bookName = bibqml::bookName(*workflowScripture_, ref.book())]()
        { provideEntry(ref, verseText, bookName); },
        Qt::QueuedConnection
      );
    }
  );
  executor_.connect(
    copyrightRequested_,
    [this]()
    {
      QMetaObject::invokeMethod(
        this,
        [this, copyright = bibqml::scriptureCopyright(*workflowScripture_)]()
        {
          if(scriptureCopyright_ != copyright)
          {
            scriptureCopyright_ = copyright;
            emit scriptureCopyrightChanged();
          }
        },
        Qt::QueuedConnection
      );
    }
  );
  copyrightRequested_();
}

///
///
int ScriptureListModel::rowCount(const QModelIndex& parent) const
{
  if(parent.isValid())
  {
    return 0;
  }
  return static_cast<int>(entries_.size());
}

///
///
QVariant ScriptureListModel::data(const QModelIndex& index, const int role) const
{
  if(!index.isValid() || index.row() < 0 || static_cast<decltype(entries_.size())>(index.row()) >= entries_.size())
  {
    return {};
  }
  const auto& entry = entries_.at(static_cast<std::size_t>(index.row()));
  switch(role)
  {
  case VerseTextRole: return entry.verseText;
  case BookIdRole: return entry.bookId;
  case BookNameRole: return entry.bookName;
  case ChapterNumberRole: return entry.chapterNumber;
  case VerseNumberRole: return entry.verseNumber;
  case IsHeaderRole: return entry.isHeader;
  default: return {};
  }
}

///
///
QHash<int, QByteArray> ScriptureListModel::roleNames() const
{
  return {
    {    VerseTextRole,     "verseText"},
    {       BookIdRole,        "bookId"},
    {     BookNameRole,      "bookName"},
    {ChapterNumberRole, "chapterNumber"},
    {  VerseNumberRole,   "verseNumber"},
    {     IsHeaderRole,      "isHeader"},
  };
}

///
///
int ScriptureListModel::referenceRow() const
{
  return referenceRow_;
}

///
///
QString ScriptureListModel::scriptureCopyright() const
{
  return scriptureCopyright_;
}

///
///
void ScriptureListModel::resetWithReference(const QString& bookId, const int chapter, const int verse)
{
  if(workflowScripture_->scripture_count() == 0)
  {
    return;
  }
  const auto versification = workflowScripture_->versification_or_fallback({{}});
  const auto ref = toReference(versification.get(), bookId, chapter, verse);
  if(!ref)
  {
    return;
  }

  beginResetModel();
  entries_.clear();
  addEntry(*ref);
  endResetModel();
  referenceRow(0);
  loadPrevious(detail::contextSize);
  loadNext(detail::contextSize);
  emit refreshed();
}

///
///
void ScriptureListModel::loadPrevious(const int count)
{
  if(entries_.empty())
  {
    return;
  }
  const auto versificationWrapper = workflowScripture_->versification_or_fallback({{}});
  decltype(auto) versification = versificationWrapper.get();

  auto ref = entries_.front().ref;
  auto newEntries = std::vector<Entry>{};
  newEntries.reserve(static_cast<std::size_t>(count));

  std::ignore = std::ranges::all_of(
    bibstd::util::ranges::index_view_to(count),
    [&]([[maybe_unused]] auto) mutable
    {
      const auto prev = versification.prev(ref);
      if(!prev)
      {
        return false;
      }
      if(entries_.size() + newEntries.size() >= detail::maxEntries)
      {
        LOG_ERROR("max entries count exceeded: reference=\"{}\" not added", *prev);
        return false;
      }
      newEntries.push_back(makeEntry(*prev));
      ref = *prev;
      return true;
    }
  );

  if(!newEntries.empty())
  {
    const auto insertCount = static_cast<int>(newEntries.size());
    beginInsertRows(QModelIndex(), 0, insertCount - 1);
    std::ranges::for_each(newEntries, [&](auto& entry) { entries_.push_front(std::move(entry)); });
    endInsertRows();
    referenceRow(referenceRow_ + insertCount);
  }
}

///
///
void ScriptureListModel::loadNext(const int count)
{
  if(entries_.empty())
  {
    return;
  }
  const auto versificationWrapper = workflowScripture_->versification_or_fallback({{}});
  decltype(auto) versification = versificationWrapper.get();

  auto ref = entries_.back().ref;
  auto newEntries = std::vector<Entry>{};
  newEntries.reserve(static_cast<std::size_t>(count));

  std::ignore = std::ranges::all_of(
    bibstd::util::ranges::index_view_to(count),
    [&]([[maybe_unused]] auto) mutable
    {
      const auto next = versification.next(ref);
      if(!next)
      {
        return false;
      }
      if(entries_.size() + newEntries.size() >= detail::maxEntries)
      {
        LOG_ERROR("max entries count exceeded: reference=\"{}\" not added", *next);
        return false;
      }
      newEntries.push_back(makeEntry(*next));
      ref = *next;
      return true;
    }
  );

  if(!newEntries.empty())
  {
    const auto insertCount = static_cast<int>(newEntries.size());
    const auto startRow = static_cast<int>(entries_.size());
    beginInsertRows(QModelIndex(), startRow, startRow + insertCount - 1);
    std::ranges::for_each(newEntries, [&](auto& entry) { entries_.push_back(std::move(entry)); });
    endInsertRows();
  }
}

///
///
void ScriptureListModel::clear()
{
  beginResetModel();
  entries_.clear();
  endResetModel();
  referenceRow(0);
}

///
///
void ScriptureListModel::disconnect()
{
  executor_.disconnect();
}

///
///
void ScriptureListModel::referenceRow(const int row)
{
  if(referenceRow_ == row)
  {
    return;
  }
  referenceRow_ = row;
  emit referenceRowChanged();
}

///
///
QString ScriptureListModel::fetchPassage(const bibstd::bible::reference& ref) const
{
  auto params =
    bibstd::workflow::workflow_scripture::passage_params::value_type{.reference = ref, .scripture_name = std::nullopt};
  auto result = workflowScripture_->passage(params);
  if(result)
  {
    return QString::fromStdString(result->passage.content);
  }
  return QString{"..."};
}

///
///
ScriptureListModel::Entry ScriptureListModel::makeEntry(const bibstd::bible::reference& ref)
{
  const auto bookId = bibstd::util::enum_name(ref.book());
  const auto bookIdText = QString::fromLatin1(bookId.data(), static_cast<qsizetype>(bookId.size()));
  // Placeholders until provideEntry fills them in
  entryRequested_(ref); // Queued into `this`'s thread
  return Entry{
    .ref = ref,
    .verseText = QString{"..."},
    .bookId = bookIdText,
    .bookName = QString{"..."},
    .chapterNumber = ref.chapter().value,
    .verseNumber = ref.verse().value,
    .isHeader = ref.verse() == decltype(ref.verse()){1},
  };
}

///
///
void ScriptureListModel::provideEntry(const bibstd::bible::reference& ref, const QString& verseText, const QString& bookName)
{
  const auto it = std::ranges::find(entries_, ref, &Entry::ref);
  if(it == std::ranges::end(entries_))
  {
    return;
  }
  it->verseText = verseText;
  it->bookName = bookName;
  const auto row = static_cast<int>(std::ranges::distance(std::ranges::begin(entries_), it));
  emit dataChanged(index(row, 0), index(row, 0));
}

///
///
void ScriptureListModel::refresh()
{
  std::ranges::for_each(entries_, [this](const Entry& entry) { entryRequested_(entry.ref); });
  copyrightRequested_();
}

///
///
void ScriptureListModel::addEntry(const bibstd::bible::reference& ref)
{
  if(entries_.size() >= detail::maxEntries)
  {
    LOG_ERROR("max entries count exceeded: reference=\"{}\" not added", ref);
    return;
  }
  entries_.emplace_back(makeEntry(ref));
}

} // namespace bibqml
