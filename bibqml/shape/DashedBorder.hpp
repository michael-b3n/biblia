#pragma once

#include <QList>
#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace bibqml
{

///
/// QML DashedBorder.
/// This class provides the dash pattern that lets a single segment travel along the border of a
/// rounded rectangle. Qt measures dashes in multiples of the stroke width, so the pattern and the
/// offset of a full turn are given in that unit as well.
///
class DashedBorder : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  // Border the pattern is computed for
  Q_PROPERTY(qreal borderWidth MEMBER borderWidth_ NOTIFY changed)
  Q_PROPERTY(qreal borderHeight MEMBER borderHeight_ NOTIFY changed)
  Q_PROPERTY(qreal radius MEMBER radius_ NOTIFY changed)
  Q_PROPERTY(qreal strokeWidth MEMBER strokeWidth_ NOTIFY changed)
  Q_PROPERTY(qreal segmentRatio MEMBER segmentRatio_ NOTIFY changed)
  Q_PROPERTY(QList<qreal> dashPattern READ dashPattern NOTIFY changed)
  Q_PROPERTY(qreal period READ period NOTIFY changed)

public: // Structors
  explicit DashedBorder(QObject* parent = nullptr);
  ~DashedBorder() noexcept override;

public: // Accessors
  ///
  /// \return length of the travelling segment followed by the gap covering the rest of the border
  ///
  QList<qreal> dashPattern() const;

  ///
  /// \return dash offset the segment travelled once around the border at
  ///
  qreal period() const;

signals:
  void changed();

private: // Implementation
  ///
  /// \return border length in multiples of the stroke width
  ///
  qreal strokes() const;

private: // Variables
  qreal borderWidth_{0.0};
  qreal borderHeight_{0.0};
  qreal radius_{0.0};
  qreal strokeWidth_{1.0};
  qreal segmentRatio_{0.25};
};

} // namespace bibqml
