#include "qml/helpers/Placement.hpp"

#include <QGuiApplication>
#include <QScreen>

#include <algorithm>
#include <array>

namespace verselens::qml
{
namespace
{

///
/// \return area both rects share
///
auto intersectionArea(const QRectF& first, const QRectF& second) -> qreal
{
  const auto shared = first.intersected(second);
  return shared.isEmpty() ? 0.0 : shared.width() * shared.height();
}

} // namespace

///
///
Placement::Placement(QObject* parent)
  : QObject{parent}
{
}

///
///
Placement::~Placement() noexcept = default;

///
///
QRectF Placement::screenGeometryAt(const QPointF& position) const
{
  const auto* const screen = QGuiApplication::screenAt(position.toPoint());
  const auto* const fallback = QGuiApplication::primaryScreen();
  if(screen != nullptr)
  {
    return screen->geometry();
  }
  return (fallback != nullptr) ? QRectF{fallback->geometry()} : QRectF{};
}

///
///
QRectF Placement::screenGeometryOf(const QRectF& rect) const
{
  return screenGeometryAt(rect.center());
}

///
///
QRectF Placement::grown(const QRectF& source, const qreal amount) const
{
  return source.isEmpty() ? source : source.adjusted(-amount, -amount, amount, amount);
}

///
///
QRectF Placement::insideScreen(const QRectF& target, const QRectF& screen) const
{
  return {
    std::clamp(target.x(), screen.left(), std::max(screen.left(), screen.right() - target.width())),
    std::clamp(target.y(), screen.top(), std::max(screen.top(), screen.bottom() - target.height())),
    target.width(),
    target.height()
  };
}

///
///
QRectF Placement::centeredSquare(const QPointF& center, const qreal size, const QRectF& screen) const
{
  return insideScreen(QRectF{center.x() - (size / 2.0), center.y() - (size / 2.0), size, size}, screen);
}

///
///
QRectF Placement::clippedToScreen(
  const QRectF& target, const QRectF& screen, const qreal minimalWidth, const qreal minimalHeight
) const
{
  const auto left = std::max(target.left(), screen.left());
  const auto top = std::max(target.top(), screen.top());
  const auto right = std::min(target.right(), screen.right());
  const auto bottom = std::min(target.bottom(), screen.bottom());
  return {left, top, std::max(right - left, minimalWidth), std::max(bottom - top, minimalHeight)};
}

///
///
QRectF Placement::placedBeside(const QRectF& target, const QRectF& blocked, const qreal clearance, const QRectF& screen) const
{
  return besideRect(insideScreen(target, screen), grown(blocked, clearance), screen);
}

///
///
QPointF Placement::borderPointTowards(const QRectF& rect, const qreal gap, const QRectF& towards) const
{
  const auto grownRect = grown(rect, gap);
  const auto position = towards.center();
  return {
    std::clamp(position.x(), grownRect.left(), grownRect.right()), std::clamp(position.y(), grownRect.top(), grownRect.bottom())
  };
}

///
///
QRectF Placement::besideRect(const QRectF& target, const QRectF& blocked, const QRectF& screen) const
{
  auto best = target;
  auto bestOverlap = intersectionArea(target, blocked);
  auto bestDistance = 0.0;
  if(bestOverlap == 0.0)
  {
    return target;
  }

  const auto size = target.size();
  const auto candidates = std::array{
    QRectF{QPointF{blocked.left() - size.width(), target.y()}, size},
    QRectF{              QPointF{blocked.right(), target.y()}, size},
    QRectF{QPointF{target.x(), blocked.top() - size.height()}, size},
    QRectF{             QPointF{target.x(), blocked.bottom()}, size}
  };
  for(const auto& candidate : candidates)
  {
    const auto moved = insideScreen(candidate, screen);
    const auto overlap = intersectionArea(moved, blocked);
    const auto distance = std::abs(moved.x() - target.x()) + std::abs(moved.y() - target.y());
    if(overlap < bestOverlap || (overlap == bestOverlap && distance < bestDistance))
    {
      best = moved;
      bestOverlap = overlap;
      bestDistance = distance;
    }
  }
  return best;
}

} // namespace verselens::qml
