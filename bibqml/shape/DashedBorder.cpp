#include "bibqml/shape/DashedBorder.hpp"

#include <algorithm>
#include <numbers>

namespace bibqml
{

///
///
DashedBorder::DashedBorder(QObject* parent)
  : QObject{parent}
{
}

///
///
DashedBorder::~DashedBorder() noexcept = default;

///
///
QList<qreal> DashedBorder::dashPattern() const
{
  const auto total = strokes();
  const auto segment = total * std::clamp(segmentRatio_, 0.0, 1.0);
  return {segment, total - segment};
}

///
///
qreal DashedBorder::period() const
{
  return strokes();
}

///
///
qreal DashedBorder::strokes() const
{
  // The four corner arcs of a rounded rectangle replace eight radii by one full circle.
  const auto length = (2.0 * (borderWidth_ + borderHeight_)) - ((8.0 - (2.0 * std::numbers::pi)) * radius_);
  return std::max(0.0, length) / std::max(strokeWidth_, 1.0);
}

} // namespace bibqml
