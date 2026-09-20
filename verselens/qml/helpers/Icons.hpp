#pragma once

#include "bibstd/util/non_owning_ptr.hpp"

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

namespace verselens::qml
{
namespace detail
{

///
/// Helper function to build the source url of an icon resource, \p name is its file name inside the qml resource folder.
/// \return The corresponding resource url
///
auto toIconUrl(std::string_view name) -> QString;

} // namespace detail

///
/// QML Icons singleton.
/// This class provides application wide icon sources.
/// Keeping the resource prefix in one place avoids repeating
/// the module path in every call site.
///
class Icons final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QString add MEMBER add_ CONSTANT)
  Q_PROPERTY(QString bell MEMBER bell_ CONSTANT)
  Q_PROPERTY(QString bellRinging MEMBER bellRinging_ CONSTANT)
  Q_PROPERTY(QString checkMark MEMBER checkMark_ CONSTANT)
  Q_PROPERTY(QString close MEMBER close_ CONSTANT)
  Q_PROPERTY(QString copyright MEMBER copyright_ CONSTANT)
  Q_PROPERTY(QString dragHandle MEMBER dragHandle_ CONSTANT)
  Q_PROPERTY(QString loading MEMBER loading_ CONSTANT)
  Q_PROPERTY(QString openInBrowser MEMBER openInBrowser_ CONSTANT)
  Q_PROPERTY(QString pin MEMBER pin_ CONSTANT)
  Q_PROPERTY(QString pinFilled MEMBER pinFilled_ CONSTANT)
  Q_PROPERTY(QString play MEMBER play_ CONSTANT)
  Q_PROPERTY(QString remove MEMBER remove_ CONSTANT)
  Q_PROPERTY(QString settings MEMBER settings_ CONSTANT)
  Q_PROPERTY(QString stop MEMBER stop_ CONSTANT)

public: // Structors
  explicit Icons(bibstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~Icons() noexcept override;

private: // Variables
  QString add_{detail::toIconUrl("add.svg")};
  QString bell_{detail::toIconUrl("bell.svg")};
  QString bellRinging_{detail::toIconUrl("bell_ringing.svg")};
  QString checkMark_{detail::toIconUrl("check_mark.svg")};
  QString close_{detail::toIconUrl("close.svg")};
  QString copyright_{detail::toIconUrl("copyright.svg")};
  QString dragHandle_{detail::toIconUrl("drag_handle.svg")};
  QString loading_{detail::toIconUrl("loading.svg")};
  QString openInBrowser_{detail::toIconUrl("open_in_browser.svg")};
  QString pin_{detail::toIconUrl("pin.svg")};
  QString pinFilled_{detail::toIconUrl("pin_filled.svg")};
  QString play_{detail::toIconUrl("play.svg")};
  QString remove_{detail::toIconUrl("remove.svg")};
  QString settings_{detail::toIconUrl("settings.svg")};
  QString stop_{detail::toIconUrl("stop.svg")};
};

} // namespace verselens::qml
