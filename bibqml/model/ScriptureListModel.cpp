#include "bibqml/model/ScriptureListModel.hpp"
#include "bibqml/util/ScriptureAccess.hpp"

#include <bibstd/bible/versification.hpp>
#include <bibstd/framework/setting.hpp>
#include <bibstd/util/enum.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/util/ranges.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <sfsm/sfsm.hpp>

#include <algorithm>
#include <functional>
#include <limits>
#include <optional>
#include <ranges>
#include <tuple>
#include <utility>

namespace bibqml
{
namespace
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

///
/// Step of a versification from a verse to the one next to it.
///
using Step = std::optional<bibstd::bible::reference> (bibstd::bible::versification::*)(const bibstd::bible::reference&) const;

///
/// Collect the verses a step leads to from \p ref on, \p ref itself is left out.
/// \return up to \p count verses in the order they were reached
///
[[nodiscard]] auto walk(
  const bibstd::bible::versification& versification, const Step step, bibstd::bible::reference ref, const int count
) -> std::vector<bibstd::bible::reference>
{
  auto refs = std::vector<bibstd::bible::reference>{};
  std::ignore = std::ranges::all_of(
    bibstd::util::ranges::index_view_to(count),
    [&]([[maybe_unused]] auto)
    {
      const auto reached = std::invoke(step, versification, ref);
      if(reached)
      {
        refs.push_back(*reached);
        ref = *reached;
      }
      return reached.has_value();
    }
  );
  return refs;
}

///
/// \return up to \p count verses before \p ref, in reading order
///
[[nodiscard]] auto versesBefore(
  const bibstd::bible::versification& versification, const bibstd::bible::reference& ref, const int count
) -> std::vector<bibstd::bible::reference>
{
  auto refs = walk(versification, &bibstd::bible::versification::prev, ref, count);
  std::ranges::reverse(refs);
  return refs;
}

///
/// \return up to \p count verses after \p ref, in reading order
///
[[nodiscard]] auto versesAfter(
  const bibstd::bible::versification& versification, const bibstd::bible::reference& ref, const int count
) -> std::vector<bibstd::bible::reference>
{
  return walk(versification, &bibstd::bible::versification::next, ref, count);
}

///
/// Read the text of a verse from the scripture of the setting "scripture.name". Blocks while a script answers.
/// \return markup of the verse, or as plain text why there is none
///
[[nodiscard]] auto verseText(const bibstd::workflow::workflow_scripture& workflowScripture, const bibstd::bible::reference& ref)
  -> QString
{
  const auto params =
    bibstd::workflow::workflow_scripture::passage_params::value_type{.reference = ref, .scripture_name = std::nullopt};
  const auto result = workflowScripture.passage(params);
  return result ? QString::fromStdString(result->passage.content) : QString::fromStdString(result.error()).toHtmlEscaped();
}

} // namespace

///
/// Builds the state machine of the loads. The model takes one load at a time, the thread pool
/// fetches them one after another anyway.
///
struct ScriptureListModel::MachineBuilder final
{
  ///
  /// \return state machine of \p self, starting in s_idle
  ///
  [[nodiscard]] static auto build(ScriptureListModel& self)
  {
    using sfsm::action, sfsm::guard, sfsm::make_transition;

    // Guards
    const auto wanted = [](const sm::e_loaded& event) { return !event.token.stop_requested(); };
    const auto resetOnTheWay = [](const sm::s_loading& loading) { return loading.load.kind == LoadKind::Reset; };
    const auto hasRows = [&self]() { return !self.entries_.empty(); };
    const auto fetchReset = [&self](const sm::e_reset& event, sm::s_loading& target)
    {
      target.load = event.load;
      self.startFetch(target);
    };
    const auto fetchPage = [&self](const sm::e_page& event, sm::s_loading& target)
    {
      target.load = event.load;
      self.startFetch(target);
    };
    const auto fetchAgain = [&self](sm::s_loading& loading) { self.startFetch(loading); };
    const auto fetchRowsAgain = [&self](sm::s_loading& loading)
    {
      loading.load = self.reloadOfRows();
      self.startFetch(loading);
    };
    const auto apply = [&self](sm::s_loading& loading, const sm::e_loaded& event)
    { self.applyLoad(loading.load, event.entries); };
    const auto stop = [](sm::s_loading& loading) { loading.stop.request_stop(); };

    // clang-format off
    return sfsm::sfsm{
      sfsm::states<sm::s_idle, sm::s_loading>{{}, {}},
      sfsm::on_exit<sm::s_loading>(stop),

      make_transition<sm::s_idle,    sm::e_reset,             sm::s_loading>(action(fetchReset)),
      make_transition<sm::s_loading, sm::e_reset,             sm::s_loading>(action(fetchReset)),
      make_transition<sm::s_idle,    sm::e_page,              sm::s_loading>(action(fetchPage)),
      make_transition<sm::s_loading, sm::e_loaded,            sm::s_idle   >(guard(wanted), action(apply)),
      make_transition<sm::s_loading, sm::e_clear,             sm::s_idle   >(),
      make_transition<sm::s_loading, sm::e_scripture_changed, sm::s_loading>(guard(resetOnTheWay), action(fetchAgain)),
      make_transition<sm::s_loading, sm::e_scripture_changed, sm::s_loading>(action(fetchRowsAgain)),
      make_transition<sm::s_idle,    sm::e_scripture_changed, sm::s_loading>(guard(hasRows), action(fetchRowsAgain))
    };
    // clang-format on
  }
};

///
/// Holds the state machine of the loads, whose type cannot be named in the header.
///
class ScriptureListModel::MachineHolder final
{
  // Variables
  decltype(MachineBuilder::build(std::declval<ScriptureListModel&>())) sm_;

public: // Structors
  explicit MachineHolder(ScriptureListModel& self)
    : sm_{MachineBuilder::build(self)}
  {
  }

public: // Operators
  ///
  /// \return pointer to the state machine, reached as (*machine_)->
  ///
  [[nodiscard]] auto operator->() -> bibstd::util::non_owning_ptr<decltype(sm_)> { return &sm_; }
};

///
///
ScriptureListModel::ScriptureListModel(
  std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture, const bibstd::util::non_owning_ptr<QObject> parent
)
  : QAbstractListModel{parent}
  , workflowScripture_{std::move(workflowScripture)}
  , machine_{std::make_unique<MachineHolder>(*this)}
{
  // The rows hold the texts of one scripture, another one or other scripts fetch them again
  const auto changed = [this]() { post([this]() { applyScriptureChange(); }); };
  workflowScripture_->settings().scripture_name->connect_queued(
    &bibstd::framework::setting_signals::value_changed, changed, executor_
  );
  workflowScripture_->connect_queued(&bibstd::workflow::workflow_scripture_sigs::scriptures_changed, changed, executor_);

  // Fetching blocks while a script answers, so it is left to the thread pool
  executor_.connect(loadRequested_, [this](const Load& load, const std::stop_token& token) { fetchLoad(load, token); });
  executor_.connect(copyrightRequested_, [this]() { fetchCopyright(); });
  copyrightRequested_();
}

///
///
ScriptureListModel::~ScriptureListModel() noexcept = default;

///
///
int ScriptureListModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(entries_.size());
}

///
///
QVariant ScriptureListModel::data(const QModelIndex& index, const int role) const
{
  if(!index.isValid() || std::cmp_greater_equal(index.row(), entries_.size()))
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
  const auto versificationWrapper = workflowScripture_->versification_or_fallback({{}});
  decltype(auto) versification = versificationWrapper.get();
  const auto ref = toReference(versification, bookId, chapter, verse);
  if(!ref)
  {
    return;
  }

  auto refs = versesBefore(versification, *ref, contextSize);
  const auto row = static_cast<int>(refs.size());
  refs.push_back(*ref);
  refs.append_range(versesAfter(versification, *ref, contextSize));

  std::ignore = dispatch(
    sm::e_reset{
      Load{.kind = LoadKind::Reset, .refs = std::move(refs), .referenceRow = row}
  }
  );
}

///
///
void ScriptureListModel::loadPrevious(const int count)
{
  if(entries_.empty())
  {
    return;
  }
  const auto versification = workflowScripture_->versification_or_fallback({{}});
  auto refs = versesBefore(versification.get(), entries_.front().ref, count);
  if(!refs.empty())
  {
    std::ignore = dispatch(
      sm::e_page{
        Load{.kind = LoadKind::Previous, .refs = std::move(refs)}
    }
    );
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
  const auto versification = workflowScripture_->versification_or_fallback({{}});
  auto refs = versesAfter(versification.get(), entries_.back().ref, count);
  if(!refs.empty())
  {
    std::ignore = dispatch(
      sm::e_page{
        Load{.kind = LoadKind::Next, .refs = std::move(refs)}
    }
    );
  }
}

///
///
void ScriptureListModel::clear()
{
  std::ignore = dispatch(sm::e_clear{});
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
void ScriptureListModel::scriptureCopyright(const QString& copyright)
{
  if(scriptureCopyright_ == copyright)
  {
    return;
  }
  scriptureCopyright_ = copyright;
  emit scriptureCopyrightChanged();
}

///
///
template<typename Event>
bool ScriptureListModel::dispatch(const Event& event)
{
  return (*machine_)->process_event(event);
}

///
///
void ScriptureListModel::provideLoad(const std::stop_token& token, std::vector<Entry> entries)
{
  if(dispatch(sm::e_loaded{.token = token, .entries = std::move(entries)}))
  {
    // Emitted once the machine is idle again, so a view asking right away is not refused
    emit loaded();
  }
}

///
///
void ScriptureListModel::applyScriptureChange()
{
  std::ignore = dispatch(sm::e_scripture_changed{});
  copyrightRequested_();
}

///
///
void ScriptureListModel::resetRows(std::vector<Entry> entries, const int row)
{
  beginResetModel();
  entries_.assign_range(entries | std::views::as_rvalue);
  endResetModel();
  referenceRow(row);
  emit refreshed();
}

///
///
void ScriptureListModel::prependRows(std::vector<Entry> entries)
{
  if(!hasRoomFor(entries.size()))
  {
    return;
  }
  const auto count = static_cast<int>(entries.size());
  beginInsertRows(QModelIndex(), 0, count - 1);
  entries_.prepend_range(entries | std::views::as_rvalue);
  endInsertRows();
  referenceRow(referenceRow_ + count);
}

///
///
void ScriptureListModel::appendRows(std::vector<Entry> entries)
{
  if(!hasRoomFor(entries.size()))
  {
    return;
  }
  const auto first = static_cast<int>(entries_.size());
  beginInsertRows(QModelIndex(), first, first + static_cast<int>(entries.size()) - 1);
  entries_.append_range(entries | std::views::as_rvalue);
  endInsertRows();
}

///
///
void ScriptureListModel::reloadRows(std::vector<Entry> entries)
{
  // The machine took in no other load since this one was requested, so the entries match the rows one by one
  entries_.assign_range(entries | std::views::as_rvalue);
  emit dataChanged(index(0, 0), index(rowCount() - 1, 0));
}

///
///
bool ScriptureListModel::hasRoomFor(const std::size_t count) const
{
  const auto fits = entries_.size() + count <= maxEntries;
  if(!fits)
  {
    LOG_ERROR("max entries count exceeded: {} verses not added", count);
  }
  return fits;
}

///
///
void ScriptureListModel::startFetch(sm::s_loading& loading)
{
  loading.stop = std::stop_source{};
  // Queued into the thread pool, \see fetchLoad
  loadRequested_(loading.load, loading.stop.get_token());
}

///
///
void ScriptureListModel::applyLoad(const Load& load, std::vector<Entry> entries)
{
  switch(load.kind)
  {
  case LoadKind::Reset: resetRows(std::move(entries), load.referenceRow); break;
  case LoadKind::Previous: prependRows(std::move(entries)); break;
  case LoadKind::Next: appendRows(std::move(entries)); break;
  case LoadKind::Reload: reloadRows(std::move(entries)); break;
  }
}

///
///
ScriptureListModel::Load ScriptureListModel::reloadOfRows() const
{
  return Load{.kind = LoadKind::Reload, .refs = entries_ | std::views::transform(&Entry::ref) | std::ranges::to<std::vector>()};
}

///
///
template<typename Task>
void ScriptureListModel::post(Task&& task)
{
  QMetaObject::invokeMethod(this, std::forward<Task>(task), Qt::QueuedConnection);
}

///
///
void ScriptureListModel::fetchLoad(const Load& load, const std::stop_token& token)
{
  auto entries = std::vector<Entry>{};
  entries.reserve(load.refs.size());
  for(const auto& ref : load.refs)
  {
    // A script may take seconds per verse, a stopped load must not keep the next one waiting
    if(token.stop_requested())
    {
      return;
    }
    entries.push_back(fetchEntry(ref));
  }
  post([this, token, entries = std::move(entries)]() mutable { provideLoad(token, std::move(entries)); });
}

///
///
void ScriptureListModel::fetchCopyright()
{
  post([this, copyright = bibqml::scriptureCopyright(*workflowScripture_)]() { scriptureCopyright(copyright); });
}

///
///
ScriptureListModel::Entry ScriptureListModel::fetchEntry(const bibstd::bible::reference& ref) const
{
  const auto bookId = bibstd::util::enum_name(ref.book());
  return Entry{
    .ref = ref,
    .verseText = verseText(*workflowScripture_, ref),
    .bookId = QString::fromLatin1(bookId.data(), static_cast<qsizetype>(bookId.size())),
    .bookName = bookName(*workflowScripture_, ref.book()),
    .chapterNumber = ref.chapter().value,
    .verseNumber = ref.verse().value,
    .isHeader = ref.verse() == decltype(ref.verse()){1},
  };
}

} // namespace bibqml
