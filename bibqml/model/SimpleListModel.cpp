#include "bibqml/model/SimpleListModel.hpp"

#include <bibstd/util/log.hpp>

#include <QJSValue>

#include <algorithm>

namespace bibqml
{

///
///
SimpleListModel::SimpleListModel(const bibstd::util::non_owning_ptr<QObject> parent)
  : QAbstractListModel{parent}
{
}

///
///
SimpleListModel::~SimpleListModel() noexcept = default;

///
///
int SimpleListModel::rowCount(const QModelIndex& parent) const
{
  if(parent.isValid())
  {
    return 0;
  }
  return static_cast<int>(entries_.size());
}

///
///
QVariant SimpleListModel::data(const QModelIndex& index, const int role) const
{
  if(!index.isValid() || index.row() < 0 || static_cast<decltype(entries_.size())>(index.row()) >= entries_.size())
  {
    return {};
  }
  const auto& entry = entries_.at(static_cast<std::size_t>(index.row()));
  switch(role)
  {
  case ValueRole: return entry;
  default: return {};
  }
}

///
///
QHash<int, QByteArray> SimpleListModel::roleNames() const
{
  return {
    {ValueRole, "value"}
  };
}

///
///
bool SimpleListModel::setData(const QModelIndex& index, const QVariant& value, const int role)
{
  if(!index.isValid() || index.row() < 0 || static_cast<decltype(entries_.size())>(index.row()) >= entries_.size())
  {
    return false;
  }
  switch(role)
  {
  case ValueRole:
  {
    entries_.at(static_cast<std::size_t>(index.row())) = value;
    emit dataChanged(index, index, {role});
    return true;
  }
  default: return false;
  }
}

///
///
Qt::ItemFlags SimpleListModel::flags([[maybe_unused]] const QModelIndex& /*index*/) const
{
  return Qt::NoItemFlags;
}

///
///
QVariantList SimpleListModel::entries() const
{
  return QVariantList{std::begin(entries_), std::end(entries_)};
}

///
///
int SimpleListModel::indexOfValue(const QVariant& value) const
{
  static constexpr auto asText = [](const QVariant& variant)
  { return variant.isValid() && !variant.isNull() ? variant.toString() : QString{}; };

  const auto text = asText(value);
  const auto found = std::ranges::find_if(entries_, [&](const auto& entry) { return asText(entry) == text; });
  return found != std::ranges::cend(entries_) ? static_cast<int>(std::ranges::distance(std::cbegin(entries_), found)) : -1;
}

///
///
bool SimpleListModel::remove(const QModelIndex& index)
{
  if(!index.isValid() || index.row() < 0 || static_cast<decltype(entries_.size())>(index.row()) >= entries_.size())
  {
    return false;
  }
  beginRemoveRows(QModelIndex(), index.row(), index.row());
  entries_.erase(std::next(std::begin(entries_), static_cast<std::size_t>(index.row())));
  endRemoveRows();
  return true;
}

///
///
void SimpleListModel::prepend(const QVariant& entry)
{
  beginInsertRows(QModelIndex(), 0, 0);
  entries_.insert(std::begin(entries_), entry);
  endInsertRows();
}
///
///
void SimpleListModel::append(const QVariant& entry)
{
  const auto row = static_cast<int>(entries_.size());
  beginInsertRows(QModelIndex(), row, row);
  entries_.push_back(entry);
  endInsertRows();
}

///
///
bool SimpleListModel::move(const QModelIndex& from, const QModelIndex& to)
{
  if(
    !from.isValid() || !to.isValid() || from.row() < 0 || to.row() < 0 ||
    static_cast<decltype(entries_.size())>(from.row()) >= entries_.size() ||
    static_cast<decltype(entries_.size())>(to.row()) >= entries_.size()
  )
  {
    return false;
  }

  const int fromRow = from.row();
  const int toRow = to.row();

  if(fromRow == toRow)
  {
    return false;
  }

  // beginMoveRows() uses "insert before" semantics for destinationChild.
  // Moving a row further down within the same parent requires the target
  // to be one past the desired final index, since the source row is
  // removed first, shifting everything after it back by one.
  const int destinationChild = (toRow > fromRow) ? toRow + 1 : toRow;

  if(!beginMoveRows(QModelIndex(), fromRow, fromRow, QModelIndex(), destinationChild))
  {
    return false;
  }

  auto item = std::move(entries_.at(static_cast<std::size_t>(fromRow)));
  entries_.erase(entries_.begin() + fromRow);
  entries_.insert(entries_.begin() + toRow, std::move(item));

  endMoveRows();
  return true;
}

///
///
bool SimpleListModel::replace(const QVariant& entries)
{
  /// Try to convert a QJSValue type inside a QVariant to a proper QVariant type.
  /// This is needed since QML might provide special Java script types that do
  /// not belong to a non user meta type.
  /// \returns QVariant type that is converted to QVariant if it was a user type.
  static constexpr auto tryNormalizeQVariant = [](const QVariant& variant) -> QVariant
  {
    const auto typeId = variant.metaType().id();
    if(typeId >= QMetaType::User && variant.canConvert<QJSValue>())
    {
      // came through as a JS array wrapped in QJSValue — unwrap it properly
      return variant.value<QJSValue>().toVariant();
    }
    return variant;
  };

  const auto normalized = tryNormalizeQVariant(entries);
  if(!normalized.canConvert<QVariantList>())
  {
    LOG_ERROR("replace failed: entries type invalid, QVariantList required");
    return false;
  }
  else
  {
    const auto& list = normalized.toList();
    beginResetModel();
    entries_.clear();
    std::ranges::for_each(list, [&](const auto& entry) { entries_.push_back(entry); });
    if(entries_.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
      LOG_WARN("replace incomplete: entries count exceeds max int");
      const auto begin = std::next(std::begin(entries_), static_cast<std::size_t>(std::numeric_limits<int>::max()));
      entries_.erase(begin, std::end(entries_));
    }
    endResetModel();
    return true;
  }
}

///
///
void SimpleListModel::clear()
{
  beginResetModel();
  entries_.clear();
  endResetModel();
}

} // namespace bibqml
