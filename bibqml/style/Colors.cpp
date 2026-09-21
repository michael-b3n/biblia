#include "bibqml/style/Colors.hpp"

#include <algorithm>

namespace bibqml
{
namespace detail
{

///
///
auto toQColor(std::string color) -> QColor
{
  static constexpr auto rgba_size = std::string_view{"#RRGGBBAA"}.size();
  if(color.size() == rgba_size)
  {
    // convert from #RRGGBBAA to #AARRGGBB format
    std::ranges::rotate(color.begin() + 1, color.end() - 2, color.end());
  }
  return {color.data()};
}

} // namespace detail

///
///
Colors::Colors(QObject* parent)
  : QObject(parent)
{
}

///
///
Colors::~Colors() noexcept = default;

} // namespace bibqml
