#pragma once

#include "bibstd/util/non_owning_ptr.hpp"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace verselens::qml
{

///
/// QML Metrics singleton.
/// This class provides application wide style metrics (spacing, font sizes, radii).
///
class Metrics final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  // Font sizes
  Q_PROPERTY(int fontSizeSmall MEMBER fontSizeSmall_ CONSTANT)
  Q_PROPERTY(int fontSizeBody MEMBER fontSizeBody_ CONSTANT)
  Q_PROPERTY(int fontSizeHeading MEMBER fontSizeHeading_ CONSTANT)
  Q_PROPERTY(int fontSizeParam MEMBER fontSizeParam_ CONSTANT)

  // Spacing
  Q_PROPERTY(int spacingTiny MEMBER spacingTiny_ CONSTANT)
  Q_PROPERTY(int spacingSmall MEMBER spacingSmall_ CONSTANT)
  Q_PROPERTY(int spacingMedium MEMBER spacingMedium_ CONSTANT)
  Q_PROPERTY(int spacingLarge MEMBER spacingLarge_ CONSTANT)

  // Padding
  Q_PROPERTY(int paddingParamContent MEMBER paddingParamContent_ CONSTANT)

  // Radii
  Q_PROPERTY(int radiusSmall MEMBER radiusSmall_ CONSTANT)
  Q_PROPERTY(int radiusMedium MEMBER radiusMedium_ CONSTANT)
  Q_PROPERTY(int radiusLarge MEMBER radiusLarge_ CONSTANT)

  // Sizes
  Q_PROPERTY(int border MEMBER border_ CONSTANT)
  Q_PROPERTY(int borderThick MEMBER borderThick_ CONSTANT)
  Q_PROPERTY(int controlHeight MEMBER controlHeight_ CONSTANT)
  Q_PROPERTY(int iconSize MEMBER iconSize_ CONSTANT)
  Q_PROPERTY(int popupHeightMax MEMBER popupHeightMax_ CONSTANT)

  // Durations
  Q_PROPERTY(int durationShort MEMBER durationShort_ CONSTANT)
  Q_PROPERTY(int durationMedium MEMBER durationMedium_ CONSTANT)
  Q_PROPERTY(int durationLong MEMBER durationLong_ CONSTANT)
  Q_PROPERTY(int durationDebounce MEMBER durationDebounce_ CONSTANT)

public: // Structors
  explicit Metrics(bibstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~Metrics() noexcept override;

private: // Variables
  // Font sizes
  int fontSizeSmall_{8};
  int fontSizeBody_{9};
  int fontSizeHeading_{10};
  int fontSizeParam_{10};

  // Spacing
  int spacingTiny_{2};
  int spacingSmall_{4};
  int spacingMedium_{6};
  int spacingLarge_{8};

  // Padding
  int paddingParamContent_{6};

  // Radii
  int radiusSmall_{2};
  int radiusMedium_{4};
  int radiusLarge_{8};

  // Sizes
  int border_{1};
  int borderThick_{2};
  int controlHeight_{24};
  int iconSize_{16};
  int popupHeightMax_{200};

  // Durations in milliseconds
  int durationShort_{200};
  int durationMedium_{300};
  int durationLong_{3000};
  int durationDebounce_{500};
};

} // namespace verselens::qml
