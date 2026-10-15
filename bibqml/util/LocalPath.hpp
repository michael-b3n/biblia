#pragma once

#include <bibstd/util/non_owning_ptr.hpp>

#include <QList>
#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QUrl>

#include <filesystem>
#include <vector>

namespace bibqml
{

///
/// QML LocalPath singleton.
/// Converts between the paths the settings hold and the urls the QML layer names files and folders by.
///
class LocalPath final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public: // Static
  ///
  /// \return the local paths of \p urls as a file dialog reports them,
  /// one naming nothing on this machine is logged and left out
  ///
  [[nodiscard]] static auto toLocalPaths(const QList<QUrl>& urls) -> std::vector<std::filesystem::path>;

public: // Structors
  explicit LocalPath(bibstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~LocalPath() noexcept override;

public: // Accessors
  ///
  /// \return url of the local \p path, empty for an empty path
  ///
  Q_INVOKABLE QUrl toUrl(const QString& path) const;

  ///
  /// \return local path of \p url, empty if the url names nothing on this machine
  ///
  Q_INVOKABLE QString fromUrl(const QUrl& url) const;
};

} // namespace bibqml
