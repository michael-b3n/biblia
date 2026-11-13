#include "bibqml/style/BibIcons.hpp"

namespace bibqml
{
namespace detail
{

///
///
auto toIconUrl(std::string_view name) -> QString
{
  static constexpr auto prefix = std::string_view{"qrc:/qt/qml/BibQml/res/"};
  return QString::fromLatin1(prefix.data(), prefix.size()).append(QLatin1StringView(name.data(), name.size()));
}

} // namespace detail

///
///
BibIcons::BibIcons(arcstd::util::non_owning_ptr<QObject> parent)
  : QObject{parent}
{
}

///
///
BibIcons::~BibIcons() noexcept = default;

} // namespace bibqml
