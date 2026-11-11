#include "arcqml/util/LocalPath.hpp"

#include <arcstd/util/log.hpp>

#include <algorithm>
#include <ranges>

namespace arcqml
{

///
///
auto LocalPath::toLocalPaths(const QList<QUrl>& urls) -> std::vector<std::filesystem::path>
{
  std::ranges::for_each(
    urls | std::views::filter([](const QUrl& url) { return !url.isLocalFile(); }),
    [](const QUrl& url) { LOG_WARN("no local file: url=\"{}\"", url.toString().toStdString()); }
  );
  return urls | std::views::filter(&QUrl::isLocalFile) |
         std::views::transform([](const QUrl& url) { return std::filesystem::path{url.toLocalFile().toStdWString()}; }) |
         std::ranges::to<std::vector>();
}

///
///
LocalPath::LocalPath(arcstd::util::non_owning_ptr<QObject> parent)
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

} // namespace arcqml
