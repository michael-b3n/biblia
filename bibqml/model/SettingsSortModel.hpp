#pragma once

#include "bibqml/model/SettingsListModel.hpp"

#include <bibstd/util/non_owning_ptr.hpp>

#include <QHash>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QVariant>

namespace bibqml
{

///
/// QML proxy model putting the settings of a SettingsListModel into the order they are read in:
/// by the display name of their category first, by their own display name second. The order
/// therefore follows the language they are displayed in and not the order the backend creates
/// them in.
/// The source model only tells which segments a setting is named by. That the first of them is
/// its category is decided here, so that the view is left with nothing but displaying it.
///
class SettingsSortModel : public QSortFilterProxyModel
{
  Q_OBJECT
  QML_ELEMENT

public: // Typedefs
  ///
  /// Roles the proxy adds to the roles of its source model.
  ///
  enum Role
  {
    ///
    /// Key of the category a setting belongs to, which is what the view groups its sections by.
    /// It is the untranslated key, so that a section stays the same one while the language
    /// changes the name it is written under.
    ///
    CategoryRole = bibqml::SettingsListModel::EndRole
  };
  Q_ENUM(Role)

public: // Structors
  explicit SettingsSortModel(bibstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~SettingsSortModel() noexcept override;

public: // Overrides
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

private: // Overrides
  bool lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const override;

private: // Implementation
  ///
  /// Read the segments a setting is named by from the source model.
  /// \return segments of the setting, empty if the index holds none
  ///
  [[nodiscard]] QStringList sourceCategories(const QModelIndex& sourceIndex) const;

  ///
  /// Name of the category a setting is grouped under, as it is displayed.
  /// \return display name of the first segment, the segment itself if no display name is available
  ///
  [[nodiscard]] static QString categoryName(const QStringList& categories);

  ///
  /// Name a setting is displayed under inside its category. Only the category is told apart, all
  /// further segments name the setting itself, the view offers no deeper level of grouping.
  /// \return display name of the segments below the category
  ///
  [[nodiscard]] static QString settingName(const QStringList& categories);
};

} // namespace bibqml
