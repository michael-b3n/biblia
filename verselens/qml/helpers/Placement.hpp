#pragma once

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QtQmlIntegration/qqmlintegration.h>

namespace verselens::qml
{

///
/// QML Placement singleton.
/// This class provides the placement math of the application windows.
/// All rects and points are given in screen coordinates.
///
class Placement final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public: // Structors
  explicit Placement(QObject* parent = nullptr);
  ~Placement() noexcept override;

public: // Accessors
  ///
  /// \return geometry of the screen at the position, of the primary screen if there is none
  ///
  Q_INVOKABLE QRectF screenGeometryAt(const QPointF& position) const;

  ///
  /// \return smallest rect containing both rects
  ///
  Q_INVOKABLE QRectF united(const QRectF& first, const QRectF& second) const;

  ///
  /// An empty rect stays empty, so an unknown area is not turned into a known one.
  /// \return rect grown by the amount on every side
  ///
  Q_INVOKABLE QRectF grown(const QRectF& source, qreal amount) const;

  ///
  /// \return rect itself if a screen shows a part of it, moved onto the primary screen otherwise
  ///
  Q_INVOKABLE QRectF reachable(const QRectF& target) const;

  ///
  /// \return square of the size centered at the position, moved onto the screen
  ///
  Q_INVOKABLE QRectF centeredSquare(const QPointF& center, qreal size, const QRectF& screen) const;

  ///
  /// Move \p target onto \p screen, then to the side of \p blocked that keeps it there and is closest to it, with
  /// \p clearance around \p blocked.
  /// \return moved rect, \p target if \p blocked is unknown or no side is free
  ///
  Q_INVOKABLE QRectF placedBeside(const QRectF& target, const QRectF& blocked, qreal clearance, const QRectF& screen) const;

  ///
  /// \return point on the border of the rect grown by the gap that is closest to the center of the
  ///         rect pointing at it
  ///
  Q_INVOKABLE QPointF borderPointTowards(const QRectF& rect, qreal gap, const QRectF& towards) const;

private: // Implementation
  ///
  /// \return rect moved onto the screen, keeping its size
  ///
  QRectF insideScreen(const QRectF& target, const QRectF& screen) const;

  QRectF besideRect(const QRectF& target, const QRectF& blocked, const QRectF& screen) const;
};

} // namespace verselens::qml
