#pragma once

#include <arcstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

namespace bibqml
{
namespace detail
{

///
/// Helper function to build the source url of an icon resource, \p name is its file name inside the module resource folder.
/// \return The corresponding resource url
///
[[nodiscard]] auto toIconUrl(std::string_view name) -> QString;

} // namespace detail

///
/// QML icons singleton of the app, the icons the controls of ArcQml do not use, \see arcqml::Icons.
///
class BibIcons final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QString bell MEMBER bell_ CONSTANT)
  Q_PROPERTY(QString bellRinging MEMBER bellRinging_ CONSTANT)
  Q_PROPERTY(QString checkMark MEMBER checkMark_ CONSTANT)
  Q_PROPERTY(QString close MEMBER close_ CONSTANT)
  Q_PROPERTY(QString code MEMBER code_ CONSTANT)
  Q_PROPERTY(QString copy MEMBER copy_ CONSTANT)
  Q_PROPERTY(QString copyright MEMBER copyright_ CONSTANT)
  Q_PROPERTY(QString fileImport MEMBER fileImport_ CONSTANT)
  Q_PROPERTY(QString openInBrowser MEMBER openInBrowser_ CONSTANT)
  Q_PROPERTY(QString pin MEMBER pin_ CONSTANT)
  Q_PROPERTY(QString pinFilled MEMBER pinFilled_ CONSTANT)
  Q_PROPERTY(QString play MEMBER play_ CONSTANT)
  Q_PROPERTY(QString reload MEMBER reload_ CONSTANT)
  Q_PROPERTY(QString settings MEMBER settings_ CONSTANT)
  Q_PROPERTY(QString stop MEMBER stop_ CONSTANT)

  // Variables
  QString bell_{detail::toIconUrl("bell.svg")};
  QString bellRinging_{detail::toIconUrl("bell_ringing.svg")};
  QString checkMark_{detail::toIconUrl("check_mark.svg")};
  QString close_{detail::toIconUrl("close.svg")};
  QString code_{detail::toIconUrl("code.svg")};
  QString copy_{detail::toIconUrl("copy.svg")};
  QString copyright_{detail::toIconUrl("copyright.svg")};
  QString fileImport_{detail::toIconUrl("file_import.svg")};
  QString openInBrowser_{detail::toIconUrl("open_in_browser.svg")};
  QString pin_{detail::toIconUrl("pin.svg")};
  QString pinFilled_{detail::toIconUrl("pin_filled.svg")};
  QString play_{detail::toIconUrl("play.svg")};
  QString reload_{detail::toIconUrl("reload.svg")};
  QString settings_{detail::toIconUrl("settings.svg")};
  QString stop_{detail::toIconUrl("stop.svg")};

public: // Structors
  explicit BibIcons(arcstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~BibIcons() noexcept override;
};

} // namespace bibqml
