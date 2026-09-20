#pragma once

#include <bibstd/util/non_owning_ptr.hpp>

#include <QColor>
#include <QPainterPath>
#include <QQuickPaintedItem>
#include <QtQmlIntegration/qqmlintegration.h>

namespace verselens::qml
{

///
/// QML item that renders the frame of a titled box: a rounded rectangle whose top line is left
/// open between gapStart and gapEnd, which is where the title of the box is set into it.
/// The gap is drawn and not painted over, so that the frame also reads correctly on a
/// translucent background of the application.
///
class FrameShape : public QQuickPaintedItem
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QColor strokeColor MEMBER strokeColor_ NOTIFY strokeColorChanged)
  Q_PROPERTY(int strokeWidth MEMBER strokeWidth_ NOTIFY strokeWidthChanged)
  Q_PROPERTY(int radius MEMBER radius_ NOTIFY radiusChanged)
  Q_PROPERTY(qreal gapStart MEMBER gapStart_ NOTIFY gapStartChanged)
  Q_PROPERTY(qreal gapEnd MEMBER gapEnd_ NOTIFY gapEndChanged)

public: // Structors
  explicit FrameShape(bibstd::util::non_owning_ptr<QQuickItem> parent = nullptr);
  ~FrameShape() noexcept override;

public: // Overrides
  void paint(bibstd::util::non_owning_ptr<QPainter> painter) override;

signals:
  void strokeColorChanged();
  void strokeWidthChanged();
  void radiusChanged();
  void gapStartChanged();
  void gapEndChanged();

private: // Implementation
  ///
  /// Builds the outline, starting at the right end of the gap and running all the way around to
  /// its left end.
  /// \return The corresponding painter path
  ///
  [[nodiscard]] auto buildPath() const -> QPainterPath;

private: // Variables
  QColor strokeColor_{Qt::gray};
  int strokeWidth_{1};
  int radius_{4};
  qreal gapStart_{0.0};
  qreal gapEnd_{0.0};
};

} // namespace verselens::qml
