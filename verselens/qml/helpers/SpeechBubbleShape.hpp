#pragma once

#include <arcstd/util/non_owning_ptr.hpp>

#include <QColor>
#include <QPainterPath>
#include <QQuickPaintedItem>
#include <QtQmlIntegration/qqmlintegration.h>

namespace verselens::qml
{

///
/// QML item that renders a speech bubble shape with a triangular tail using QPainter.
/// Sizes itself to the shape's bounding box for minimal texture allocation.
///
class SpeechBubbleShape : public QQuickPaintedItem
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int radius MEMBER radius_ NOTIFY radiusChanged)
  Q_PROPERTY(int tailPositionX MEMBER tailPositionX_ NOTIFY tailPositionXChanged)
  Q_PROPERTY(int tailPositionY MEMBER tailPositionY_ NOTIFY tailPositionYChanged)
  Q_PROPERTY(int offsetToTailX MEMBER offsetToTailX_ NOTIFY offsetToTailXChanged)
  Q_PROPERTY(int offsetToTailY MEMBER offsetToTailY_ NOTIFY offsetToTailYChanged)
  Q_PROPERTY(int bubbleWidth MEMBER bubbleWidth_ NOTIFY bubbleWidthChanged)
  Q_PROPERTY(int bubbleHeight MEMBER bubbleHeight_ NOTIFY bubbleHeightChanged)
  Q_PROPERTY(bool tailVisible MEMBER tailVisible_ NOTIFY tailVisibleChanged)
  Q_PROPERTY(QColor strokeColor MEMBER strokeColor_ NOTIFY strokeColorChanged)
  Q_PROPERTY(QColor fillColor MEMBER fillColor_ NOTIFY fillColorChanged)

public: // Structors
  explicit SpeechBubbleShape(arcstd::util::non_owning_ptr<QQuickItem> parent = nullptr);
  ~SpeechBubbleShape() noexcept override;

public: // Overrides
  void paint(arcstd::util::non_owning_ptr<QPainter> painter) override;

protected: // Overrides
  void updatePolish() override;

signals:
  void radiusChanged();
  void tailPositionXChanged();
  void tailPositionYChanged();
  void offsetToTailXChanged();
  void offsetToTailYChanged();
  void bubbleWidthChanged();
  void bubbleHeightChanged();
  void tailVisibleChanged();
  void strokeColorChanged();
  void fillColorChanged();

private: // Constants
  static constexpr int strokeWidth_{2};
  static constexpr int padding_{4}; // extra padding around bounding box

private: // Implementation
  void markDirty();
  void rebuildPath();

private: // Variables
  int radius_{8};
  int tailPositionX_{0};
  int tailPositionY_{0};
  int offsetToTailX_{0};
  int offsetToTailY_{0};
  int bubbleWidth_{100};
  int bubbleHeight_{50};
  bool tailVisible_{true};
  QColor strokeColor_{Qt::gray};
  QColor fillColor_{Qt::white};

  QPainterPath path_;
  bool pathDirty_{true};
};

} // namespace verselens::qml
