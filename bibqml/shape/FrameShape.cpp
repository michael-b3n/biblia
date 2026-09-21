#include "bibqml/shape/FrameShape.hpp"

#include <QPainter>

#include <algorithm>

namespace bibqml
{

///
///
FrameShape::FrameShape(bibstd::util::non_owning_ptr<QQuickItem> parent)
  : QQuickPaintedItem{parent}
{
  setAntialiasing(true);
  const auto repaint = [this] { update(); };
  connect(this, &FrameShape::strokeColorChanged, this, repaint);
  connect(this, &FrameShape::strokeWidthChanged, this, repaint);
  connect(this, &FrameShape::radiusChanged, this, repaint);
  connect(this, &FrameShape::gapStartChanged, this, repaint);
  connect(this, &FrameShape::gapEndChanged, this, repaint);
}

///
///
FrameShape::~FrameShape() noexcept = default;

///
///
auto FrameShape::buildPath() const -> QPainterPath
{
  // The line is stroked on its middle, so the outline is inset by half of its width
  const auto inset = strokeWidth_ / 2.0;
  const auto rect = QRectF{inset, inset, width() - strokeWidth_, height() - strokeWidth_};
  const auto radius = std::min({static_cast<qreal>(radius_), rect.width() / 2.0, rect.height() / 2.0});
  const auto diameter = 2.0 * radius;

  // The gap stays between the two top corners, a title wider than the frame would open it up
  const auto gapStart = std::clamp(gapStart_, rect.left() + radius, rect.right() - radius);
  const auto gapEnd = std::clamp(gapEnd_, gapStart, rect.right() - radius);

  auto path = QPainterPath{};
  path.moveTo(gapEnd, rect.top());
  path.lineTo(rect.right() - radius, rect.top());
  path.arcTo(QRectF{rect.right() - diameter, rect.top(), diameter, diameter}, 90.0, -90.0);
  path.lineTo(rect.right(), rect.bottom() - radius);
  path.arcTo(QRectF{rect.right() - diameter, rect.bottom() - diameter, diameter, diameter}, 0.0, -90.0);
  path.lineTo(rect.left() + radius, rect.bottom());
  path.arcTo(QRectF{rect.left(), rect.bottom() - diameter, diameter, diameter}, 270.0, -90.0);
  path.lineTo(rect.left(), rect.top() + radius);
  path.arcTo(QRectF{rect.left(), rect.top(), diameter, diameter}, 180.0, -90.0);
  path.lineTo(gapStart, rect.top());
  return path;
}

///
///
void FrameShape::paint(bibstd::util::non_owning_ptr<QPainter> painter)
{
  if((painter == nullptr) || width() <= strokeWidth_ || height() <= strokeWidth_)
  {
    return;
  }

  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->strokePath(buildPath(), QPen{strokeColor_, static_cast<qreal>(strokeWidth_)});
}

} // namespace bibqml
