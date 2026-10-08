#pragma once

#include <bibstd/bible/reference.hpp>
#include <bibstd/framework/synchronized_executor.hpp>
#include <bibstd/signal/common.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <QAbstractListModel>
#include <QMetaEnum>
#include <QObject>
#include <QtQml/qqmlregistration.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <stop_token>
#include <vector>

namespace bibstd::workflow
{
// Forward declaration
class workflow_scripture;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// List model of the verses around a bible reference, further ones are loaded as the user scrolls.
/// Verses are fetched in the thread pool and enter the model with their text: a row growing
/// afterwards would move the verse a view is showing. One load is on the way at a time, \see loaded.
///
class ScriptureListModel final : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QString scriptureCopyright READ scriptureCopyright NOTIFY scriptureCopyrightChanged FINAL)
  Q_PROPERTY(int referenceRow READ referenceRow NOTIFY referenceRowChanged FINAL)

  // Typedefs
  ///
  /// Verse as the delegates of a view read it, \see Role.
  ///
  struct Entry final
  {
    bibstd::bible::reference ref;
    QString verseText;
    QString bookId;
    QString bookName;
    std::uint32_t chapterNumber;
    std::uint32_t verseNumber;
    bool isHeader;
  };

  ///
  /// What a load does with the verses it fetched.
  ///
  enum class LoadKind
  {
    // Replace all rows
    Reset,
    // Insert before the first row
    Previous,
    // Insert after the last row
    Next,
    // Replace the texts of the rows, the scripture changed
    Reload,
  };

  ///
  /// Verses to fetch for the model, in reading order.
  ///
  struct Load final
  {
    LoadKind kind;
    std::vector<bibstd::bible::reference> refs;
    int referenceRow{0};
  };

  ///
  /// States and events of the state machine of the loads, \see MachineBuilder for its transitions.
  ///
  struct sm final
  {
    // clang-format off
    // States
    struct s_idle final {};
    struct s_loading final { Load load; std::stop_source stop; };

    // Events
    struct e_reset final { Load load; };
    struct e_page final { Load load; };
    struct e_loaded final { std::stop_token token; std::vector<Entry> entries; };
    struct e_clear final {};
    struct e_scripture_changed final {};
    // clang-format on
  };
  struct MachineBuilder;
  class MachineHolder;

  // Variables
  const std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture_;
  std::deque<Entry> entries_;
  int referenceRow_{0};
  QString scriptureCopyright_;
  bibstd::signal::signal_type<void(Load, std::stop_token)> loadRequested_;
  bibstd::signal::signal_type<void()> copyrightRequested_;
  bibstd::framework::synchronized_executor executor_{bibstd::framework::thread_pool::strand_id()};
  const std::unique_ptr<MachineHolder> machine_;

public: // Typedefs
  ///
  /// Roles the delegates of the view read an entry by.
  ///
  enum Role
  {
    VerseTextRole = Qt::UserRole + 1,
    BookIdRole,
    BookNameRole,
    ChapterNumberRole,
    VerseNumberRole,
    IsHeaderRole,
  };
  Q_ENUM(Role)

public: // Structors
  explicit ScriptureListModel(
    std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture,
    bibstd::util::non_owning_ptr<QObject> parent = nullptr
  );
  ~ScriptureListModel() noexcept override;

public: // Overrides
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

public: // Accessors
  ///
  /// Row the reference of the last reset sits at. Verses loaded before it
  /// push it down, so it is the row a view positions on to show the reference.
  /// \return row of the reference, 0 while the model is empty
  ///
  [[nodiscard]] int referenceRow() const;

  ///
  /// Copyright statement of the scripture the verses are taken from.
  /// \return copyright statement, empty if the scripture does not provide one
  ///
  [[nodiscard]] QString scriptureCopyright() const;

public: // Modifiers
  ///
  /// Reset the model with a new reference. The reference is loaded with context on both sides and
  /// replaces all existing entries once it is fetched, \see referenceRow tells which row it ended up at.
  /// \note refreshed is emitted once the reset is done.
  ///
  Q_INVOKABLE void resetWithReference(const QString& bookId, int chapter, int verse);

  ///
  /// Load more verses before the current first entry. They are inserted once they are fetched,
  /// a call while another load is on the way is ignored, \see loaded tells when to ask again.
  ///
  Q_INVOKABLE void loadPrevious(int count);

  ///
  /// Load more verses after the current last entry, \see loadPrevious.
  ///
  Q_INVOKABLE void loadNext(int count);

  ///
  /// Clear all entries from the model.
  ///
  Q_INVOKABLE void clear();

  ///
  /// Disconnect all signal connections.
  /// This will stop the frontend backend communication.
  ///
  void disconnect();

signals:
  void referenceRowChanged();
  void scriptureCopyrightChanged();
  void refreshed();
  void loaded();

private: // Implementation, on the thread of the model
  void referenceRow(int row);
  void scriptureCopyright(const QString& copyright);
  template<typename Event>
  [[nodiscard]] bool dispatch(const Event& event);
  void provideLoad(const std::stop_token& token, std::vector<Entry> entries);
  void applyScriptureChange();
  void resetRows(std::vector<Entry> entries, int row);
  void prependRows(std::vector<Entry> entries);
  void appendRows(std::vector<Entry> entries);
  void reloadRows(std::vector<Entry> entries);
  [[nodiscard]] bool hasRoomFor(std::size_t count) const;

private: // Actions
  void startFetch(sm::s_loading& loading);
  void applyLoad(const Load& load, std::vector<Entry> entries);
  [[nodiscard]] Load reloadOfRows() const;

private: // Implementation, on the thread pool
  template<typename Task>
  void post(Task&& task);
  void fetchLoad(const Load& load, const std::stop_token& token);
  void fetchCopyright();
  [[nodiscard]] Entry fetchEntry(const bibstd::bible::reference& ref) const;
};

} // namespace bibqml
