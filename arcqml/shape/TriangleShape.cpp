#include "arcqml/shape/TriangleShape.hpp"

#include <QPainter>
#include <QPainterPath>

namespace arcqml
{

///
///
TriangleShape::TriangleShape(arcstd::util::non_owning_ptr<QQuickItem> parent)
  : QQuickPaintedItem{parent}
{
  setAntialiasing(true);
  connect(this, &TriangleShape::colorChanged, this, [this] { update(); });
}

///
///
TriangleShape::~TriangleShape() noexcept = default;

///
///
void TriangleShape::paint(arcstd::util::non_owning_ptr<QPainter> painter)
{
  if((painter == nullptr) || width() <= 0 || height() <= 0)
  {
    return;
  }

  auto path = QPainterPath{};
  path.moveTo(0, 0);
  path.lineTo(width(), 0);
  path.lineTo(width() / 2, height());
  path.closeSubpath();

  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->fillPath(path, color_);
}

} // namespace arcqml
