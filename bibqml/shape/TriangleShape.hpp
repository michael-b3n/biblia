#pragma once

#include <bibstd/util/non_owning_ptr.hpp>

#include <QColor>
#include <QQuickPaintedItem>
#include <QtQmlIntegration/qqmlintegration.h>

namespace bibqml
{

///
/// QML item that renders a filled triangle pointing down, spanning the whole item.
///
class TriangleShape : public QQuickPaintedItem
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QColor color MEMBER color_ NOTIFY colorChanged)

public: // Structors
  explicit TriangleShape(bibstd::util::non_owning_ptr<QQuickItem> parent = nullptr);
  ~TriangleShape() noexcept override;

public: // Overrides
  void paint(bibstd::util::non_owning_ptr<QPainter> painter) override;

signals:
  void colorChanged();

private: // Variables
  QColor color_{Qt::black};
};

} // namespace bibqml
