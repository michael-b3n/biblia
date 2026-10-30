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
/// QML proxy model ordering the settings of a SettingsListModel by the display name of their category, their first
/// segment, then by their own, so the order follows the display language.
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
