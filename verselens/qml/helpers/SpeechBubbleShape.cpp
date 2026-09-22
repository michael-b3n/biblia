#include "qml/helpers/SpeechBubbleShape.hpp"

#include <QPainter>
#include <QPen>

#include <algorithm>

namespace verselens::qml
{

///
///
SpeechBubbleShape::SpeechBubbleShape(bibstd::util::non_owning_ptr<QQuickItem> parent)
  : QQuickPaintedItem{parent}
{
  setAntialiasing(true);
  setRenderTarget(QQuickPaintedItem::FramebufferObject);
  setPerformanceHint(QQuickPaintedItem::FastFBOResizing);

  // Geometry-affecting properties trigger path rebuild
  connect(this, &SpeechBubbleShape::radiusChanged, this, &SpeechBubbleShape::markDirty);
  connect(this, &SpeechBubbleShape::tailPositionXChanged, this, &SpeechBubbleShape::markDirty);
  connect(this, &SpeechBubbleShape::tailPositionYChanged, this, &SpeechBubbleShape::markDirty);
  connect(this, &SpeechBubbleShape::offsetToTailXChanged, this, &SpeechBubbleShape::markDirty);
  connect(this, &SpeechBubbleShape::offsetToTailYChanged, this, &SpeechBubbleShape::markDirty);
  connect(this, &SpeechBubbleShape::bubbleWidthChanged, this, &SpeechBubbleShape::markDirty);
  connect(this, &SpeechBubbleShape::bubbleHeightChanged, this, &SpeechBubbleShape::markDirty);
  connect(this, &SpeechBubbleShape::tailVisibleChanged, this, &SpeechBubbleShape::markDirty);

  // Color-only changes just need a repaint
  connect(this, &SpeechBubbleShape::strokeColorChanged, this, [this] { update(); });
  connect(this, &SpeechBubbleShape::fillColorChanged, this, [this] { update(); });
}

///
///
SpeechBubbleShape::~SpeechBubbleShape() noexcept = default;

///
///
void SpeechBubbleShape::paint(bibstd::util::non_owning_ptr<QPainter> painter)
{
  if((painter == nullptr) || path_.isEmpty())
  {
    return;
  }

  painter->setRenderHint(QPainter::Antialiasing, true);

  // Translate so path coordinates (which are in parent space) map to item-local space
  painter->translate(-x(), -y());

  QPen pen(strokeColor_, strokeWidth_);
  pen.setJoinStyle(Qt::RoundJoin);
  painter->setPen(pen);
  painter->setBrush(fillColor_);
  painter->drawPath(path_);
}

///
///
void SpeechBubbleShape::updatePolish()
{
  if(!pathDirty_)
  {
    return;
  }
  pathDirty_ = false;

  rebuildPath();

  // Compute bounding rect and resize item to fit just the shape
  const auto bounds = path_.controlPointRect();
  const auto margin = strokeWidth_ + padding_;
  const auto newX = bounds.left() - margin;
  const auto newY = bounds.top() - margin;
  const auto newW = bounds.width() + (2 * margin);
  const auto newH = bounds.height() + (2 * margin);

  setX(newX);
  setY(newY);
  setWidth(newW);
  setHeight(newH);
  update();
}

///
///
void SpeechBubbleShape::markDirty()
{
  pathDirty_ = true;
  polish();
}

///
///
void SpeechBubbleShape::rebuildPath()
{
  const auto bubbleX = static_cast<double>(tailPositionX_ + offsetToTailX_);
  const auto bubbleY = static_cast<double>(tailPositionY_ + offsetToTailY_);
  const auto bw = static_cast<double>(bubbleWidth_);
  const auto bh = static_cast<double>(bubbleHeight_);
  const auto r = static_cast<double>(radius_);
  const auto tailWidth = r;

  // Edge boundaries (inside the rounded corners)
  const auto lineLeftX = bubbleX + r;
  const auto lineRightX = bubbleX + bw - r;
  const auto lineTopY = bubbleY + r;
  const auto lineBottomY = bubbleY + bh - r;

  // Determine tail direction. Without a tail the shape stays a plain rounded rectangle.
  const auto tailUp = tailVisible_ && tailPositionY_ < static_cast<int>(bubbleY);
  const auto tailDown = tailVisible_ && tailPositionY_ > static_cast<int>(bubbleY + bh);
  const auto tailLeft = tailVisible_ && !tailUp && !tailDown && tailPositionX_ < static_cast<int>(bubbleX);
  const auto tailRight = tailVisible_ && !tailUp && !tailDown && tailPositionX_ > static_cast<int>(bubbleX + bw);

  const auto tailX = static_cast<double>(tailPositionX_);
  const auto tailY = static_cast<double>(tailPositionY_);

  // Tail projection points on the bubble edge
  const auto tailProjLeftX = std::max(lineLeftX, std::min(tailX - (tailWidth / 2.0), lineRightX - tailWidth));
  const auto tailProjRightX = std::min(lineRightX, std::max(tailX + (tailWidth / 2.0), lineLeftX + tailWidth));
  const auto tailProjTopY = std::max(lineTopY, std::min(tailY - (tailWidth / 2.0), lineBottomY - tailWidth));
  const auto tailProjBottomY = std::min(lineBottomY, std::max(tailY + (tailWidth / 2.0), lineTopY + tailWidth));

  // Build the path
  path_ = QPainterPath{};
  path_.moveTo(bubbleX + r, bubbleY);

  // Top side with optional tail
  path_.lineTo(tailProjLeftX, bubbleY);
  if(tailUp)
  {
    path_.lineTo(tailX, tailY);
  }
  path_.lineTo(tailProjRightX, bubbleY);
  path_.lineTo(bubbleX + bw - r, bubbleY);

  // Top-right corner
  path_.arcTo(bubbleX + bw - (2 * r), bubbleY, 2 * r, 2 * r, 90, -90);

  // Right side with optional tail
  path_.lineTo(bubbleX + bw, tailProjTopY);
  if(tailRight)
  {
    path_.lineTo(tailX, tailY);
  }
  path_.lineTo(bubbleX + bw, tailProjBottomY);
  path_.lineTo(bubbleX + bw, bubbleY + bh - r);

  // Bottom-right corner
  path_.arcTo(bubbleX + bw - (2 * r), bubbleY + bh - (2 * r), 2 * r, 2 * r, 0, -90);

  // Bottom side with optional tail
  path_.lineTo(tailProjRightX, bubbleY + bh);
  if(tailDown)
  {
    path_.lineTo(tailX, tailY);
  }
  path_.lineTo(tailProjLeftX, bubbleY + bh);
  path_.lineTo(bubbleX + r, bubbleY + bh);

  // Bottom-left corner
  path_.arcTo(bubbleX, bubbleY + bh - (2 * r), 2 * r, 2 * r, -90, -90);

  // Left side with optional tail
  path_.lineTo(bubbleX, tailProjBottomY);
  if(tailLeft)
  {
    path_.lineTo(tailX, tailY);
  }
  path_.lineTo(bubbleX, tailProjTopY);
  path_.lineTo(bubbleX, bubbleY + r);

  // Top-left corner
  path_.arcTo(bubbleX, bubbleY, 2 * r, 2 * r, 180, -90);

  path_.closeSubpath();
}

} // namespace verselens::qml
