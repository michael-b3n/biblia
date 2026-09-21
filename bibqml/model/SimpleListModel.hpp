#pragma once

#include <bibstd/util/non_owning_ptr.hpp>

#include <QAbstractListModel>
#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QVariant>

#include <vector>

namespace bibqml
{

///
/// Simple list model holding QVariant values for a ListView.
///
class SimpleListModel : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

  // Variables
  std::vector<QVariant> entries_;

public: // Typedefs
  enum Role
  {
    ValueRole = Qt::UserRole + 1,
  };
  Q_ENUM(Role)

public: // Structors
  explicit SimpleListModel(bibstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~SimpleListModel() noexcept override;

public: // Overrides
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

public: // Accessors
  ///
  /// \return all entries as a QVariantList.
  ///
  Q_INVOKABLE QVariantList entries() const;

  ///
  /// Find the entry holding a value. Entries and the value looked up may be of different
  /// types, e.g. a number stored as text, so they are compared by their text.
  /// \return row of the first entry holding the value, -1 if no entry holds it
  ///
  Q_INVOKABLE int indexOfValue(const QVariant& value) const;

public: // Modifiers
  ///
  /// Remove the entry at the specified index from the model.
  /// \return true if the entry was removed, false otherwise
  ///
  Q_INVOKABLE bool remove(const QModelIndex& index);

  ///
  /// Prepend a new entry to the model.
  ///
  Q_INVOKABLE void prepend(const QVariant& entry);

  ///
  /// Append a new entry to the model.
  ///
  Q_INVOKABLE void append(const QVariant& entry);

  ///
  /// Move an entry from one index to another in the model.
  /// \return true if the entry was moved, false otherwise
  ///
  Q_INVOKABLE bool move(const QModelIndex& from, const QModelIndex& to);

  ///
  /// Replace all entries in the model with the specified entries.
  /// Requires that the QVariant value contains QVariantList.
  /// \return true if the entries were replaced, false otherwise
  ///
  Q_INVOKABLE bool replace(const QVariant& entries);

  ///
  /// Clear all entries from the model.
  ///
  Q_INVOKABLE void clear();
};

} // namespace bibqml
