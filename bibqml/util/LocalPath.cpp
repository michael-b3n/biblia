#include "bibqml/util/LocalPath.hpp"

namespace bibqml
{

///
///
LocalPath::LocalPath(bibstd::util::non_owning_ptr<QObject> parent)
  : QObject{parent}
{
}

///
///
LocalPath::~LocalPath() noexcept = default;

///
///
QUrl LocalPath::toUrl(const QString& path) const
{
  return QUrl::fromLocalFile(path);
}

///
///
QString LocalPath::fromUrl(const QUrl& url) const
{
  return url.toLocalFile();
}

} // namespace bibqml
