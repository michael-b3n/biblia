#pragma once

#include <bibstd/bible/reference.hpp>
#include <bibstd/signal/common.hpp>
#include <bibstd/signal/synchronized_executor.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <QAbstractListModel>
#include <QMetaEnum>
#include <QObject>
#include <QtQml/qqmlregistration.h>

#include <deque>
#include <memory>

namespace bibstd::workflow
{
// Forward declaration
class workflow_scripture;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// List model providing bible passages for a ListView.
/// Supports dynamic loading of previous/next verses as the user scrolls.
///
class ScriptureListModel final : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QString scriptureCopyright READ scriptureCopyright NOTIFY scriptureCopyrightChanged FINAL)
  Q_PROPERTY(int referenceRow READ referenceRow NOTIFY referenceRowChanged FINAL)

  // Typedefs
  ///
  /// ListModel Entry
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

  // Variables
  const std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture_;
  std::deque<Entry> entries_;
  int referenceRow_{0};
  QString scriptureCopyright_;
  bibstd::signal::signal_type<void(bibstd::bible::reference)> entryRequested_;
  bibstd::signal::signal_type<void()> copyrightRequested_;
  bibstd::signal::synchronized_executor executor_{bibstd::framework::thread_pool::strand_id()};

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
  int referenceRow() const;

  ///
  /// Copyright statement of the scripture the verses are taken from.
  /// \return copyright statement, empty if the scripture does not provide one
  ///
  QString scriptureCopyright() const;

public: // Modifiers
  ///
  /// Reset the model with a new reference. All existing entries are dropped and the reference is
  /// loaded with context on both sides, \see referenceRow tells which row it ended up at.
  /// \note refreshed is emitted once the reset is done.
  ///
  Q_INVOKABLE void resetWithReference(const QString& bookId, int chapter, int verse);

  ///
  /// Load more verses before the current first entry.
  ///
  Q_INVOKABLE void loadPrevious(int count);

  ///
  /// Load more verses after the current last entry.
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

private: // Implementation
  void referenceRow(int row);
  QString fetchPassage(const bibstd::bible::reference& ref) const;
  Entry makeEntry(const bibstd::bible::reference& ref);
  void provideEntry(const bibstd::bible::reference& ref, const QString& verseText, const QString& bookName);
  void refresh();
  void addEntry(const bibstd::bible::reference& ref);
};

} // namespace bibqml
