#include "bibqml/model/SettingsSortModel.hpp"
#include "bibqml/translation/Translations.hpp"

namespace bibqml
{
namespace
{

// Index of the segment naming the category a setting is grouped under
constexpr auto category_index = qsizetype{0};

} // anonymous namespace

///
///
SettingsSortModel::SettingsSortModel(const bibstd::util::non_owning_ptr<QObject> parent)
  : QSortFilterProxyModel{parent}
{
  setDynamicSortFilter(true);
  sort(0);
  if(const auto* const translations = Translations::instance())
  {
    // The order follows the display names, so a language change reorders the settings.
    connect(translations, &Translations::languageChanged, this, [this]() { invalidate(); });
  }
}

///
///
SettingsSortModel::~SettingsSortModel() noexcept = default;

///
///
QVariant SettingsSortModel::data(const QModelIndex& index, const int role) const
{
  if(role == CategoryRole)
  {
    return sourceCategories(mapToSource(index)).value(category_index);
  }
  return QSortFilterProxyModel::data(index, role);
}

///
///
QHash<int, QByteArray> SettingsSortModel::roleNames() const
{
  auto names = QSortFilterProxyModel::roleNames();
  names.insert(CategoryRole, "category");
  return names;
}

///
///
bool SettingsSortModel::lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const
{
  // Names are compared the way a language sorts them and not by their characters, which would
  // list every name starting with an umlaut behind the last one starting with a letter.
  // \note The comparison follows the locale of the system, not the language of the names.
  const auto lhsCategories = sourceCategories(lhs);
  const auto rhsCategories = sourceCategories(rhs);
  const auto categoryOrder = QString::localeAwareCompare(categoryName(lhsCategories), categoryName(rhsCategories));
  if(categoryOrder != 0)
  {
    return categoryOrder < 0;
  }
  return QString::localeAwareCompare(settingName(lhsCategories), settingName(rhsCategories)) < 0;
}

///
///
QStringList SettingsSortModel::sourceCategories(const QModelIndex& sourceIndex) const
{
  const auto* const source = sourceModel();
  if((source == nullptr) || !sourceIndex.isValid())
  {
    return {};
  }
  return source->data(sourceIndex, bibqml::SettingsListModel::CategoriesRole).toStringList();
}

///
///
QString SettingsSortModel::categoryName(const QStringList& categories)
{
  const auto category = categories.value(category_index);
  const auto* const translations = Translations::instance();
  return (translations != nullptr) ? translations->name(category) : category;
}

///
///
QString SettingsSortModel::settingName(const QStringList& categories)
{
  const auto belowCategory = categories.mid(category_index + 1);
  const auto* const translations = Translations::instance();
  return (translations != nullptr) ? translations->names(belowCategory) : belowCategory.join(Translations::nameSeparator);
}

} // namespace bibqml
