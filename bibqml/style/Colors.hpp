#pragma once

#include <QColor>
#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace bibqml
{
namespace detail
{

///
/// Helper function to convert a hex color string to a QColor object.
/// The input string should be in the format "#RRGGBBAA" or "#RRGGBB".
/// \return The corresponding QColor object
///
auto toQColor(std::string color) -> QColor;

} // namespace detail

///
/// QML Colors singleton.
/// This class provides the style colors of the module.
///
class Colors final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QColor green MEMBER green_ CONSTANT)
  Q_PROPERTY(QColor greenDarker MEMBER greenDarker_ CONSTANT)
  Q_PROPERTY(QColor red MEMBER red_ CONSTANT)

  Q_PROPERTY(QColor backgroundTransparent MEMBER backgroundTransparent_ NOTIFY backgroundTransparentChanged)
  Q_PROPERTY(QColor backgroundSolid MEMBER backgroundSolid_ NOTIFY backgroundSolidChanged)
  Q_PROPERTY(QColor backgroundSolidDarker MEMBER backgroundSolidDarker_ NOTIFY backgroundSolidDarkerChanged)
  Q_PROPERTY(QColor border MEMBER border_ NOTIFY borderChanged)
  Q_PROPERTY(QColor borderDarker MEMBER borderDarker_ NOTIFY borderDarkerChanged)
  Q_PROPERTY(QColor selection MEMBER selection_ NOTIFY selectionChanged)
  Q_PROPERTY(QColor pressed MEMBER pressed_ NOTIFY pressedChanged)

  Q_PROPERTY(QColor text MEMBER text_ NOTIFY textChanged)
  Q_PROPERTY(QColor verseBox MEMBER verseBox_ NOTIFY verseBoxChanged)
  Q_PROPERTY(QColor verseText MEMBER verseText_ NOTIFY verseTextChanged)
  Q_PROPERTY(QColor bookHeader MEMBER bookHeader_ NOTIFY bookHeaderChanged)
  Q_PROPERTY(QColor bookHeaderText MEMBER bookHeaderText_ NOTIFY bookHeaderTextChanged)

public: // Structors
  explicit Colors(QObject* parent = nullptr);
  ~Colors() noexcept override;

signals:
  void backgroundTransparentChanged();
  void backgroundSolidChanged();
  void backgroundSolidDarkerChanged();
  void borderChanged();
  void borderDarkerChanged();
  void selectionChanged();
  void pressedChanged();

  void textChanged();
  void verseBoxChanged();
  void verseTextChanged();
  void bookHeaderChanged();
  void bookHeaderTextChanged();

private: // Variables
  // Base colors
  QColor green_{detail::toQColor("#7db356ff")};
  QColor greenDarker_{detail::toQColor("#5a8a38ff")};

  QColor red_{detail::toQColor("#c8503cff")};

  // Layout colors
  QColor backgroundTransparent_{detail::toQColor("#fef5deed")};
  QColor backgroundSolid_{detail::toQColor("#fef5deff")};
  QColor backgroundSolidDarker_{detail::toQColor("#f0ddb2ff")};
  QColor border_{detail::toQColor("#a3835cff")};
  QColor borderDarker_{detail::toQColor("#7e6444ff")};
  QColor selection_{detail::toQColor("#f0cea8ff")};
  QColor pressed_{detail::toQColor("#d5a16dff")};

  // TextColors
  QColor text_{detail::toQColor("#2e2319ff")};
  QColor verseBox_{detail::toQColor("#7b5f42ff")};
  QColor verseText_{detail::toQColor("#fef5deff")};
  QColor bookHeader_{detail::toQColor("#8f633dff")};
  QColor bookHeaderText_{detail::toQColor("#fef5deff")};
};

} // namespace bibqml
