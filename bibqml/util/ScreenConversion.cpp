#include "bibqml/util/ScreenConversion.hpp"

#include <bibstd/math/rect.hpp>
#include <bibstd/system/screen.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/util/numeric_cast.hpp>

#include <QGuiApplication>
#include <QScreen>

#include <cmath>
#include <cstdint>

namespace bibqml
{
namespace
{

///
/// Mapping of one monitor between native and device independent pixels. The monitor is known by its
/// geometry in both systems, a position is mapped by its relative placement on the monitor.
///
struct MonitorMapping final
{
  // Variables
  bibstd::util::screen_rect_type nativeGeometry;
  QRect deviceIndependentGeometry;

  ///
  /// Scale a horizontal distance in native pixels to device independent pixels.
  /// \return distance in device independent pixels
  ///
  [[nodiscard]] auto horizontal(std::int64_t nativePixels) const -> int;

  ///
  /// Scale a vertical distance in native pixels to device independent pixels.
  /// \return distance in device independent pixels
  ///
  [[nodiscard]] auto vertical(std::int64_t nativePixels) const -> int;

  ///
  /// Map a position in native pixels that is on this monitor to device independent pixels.
  /// \return position in device independent pixels
  ///
  [[nodiscard]] auto map(const bibstd::util::screen_coordinates_type& position) const -> QPoint;
};

///
/// Scale a distance by the ratio of the sizes of a monitor in both systems.
/// \return distance in device independent pixels
///
[[nodiscard]] auto scaled(const std::int64_t nativePixels, const int deviceIndependentSize, const std::uint32_t nativeSize)
  -> int
{
  return numeric_cast<int>(
    std::lround(static_cast<qreal>(nativePixels) * static_cast<qreal>(deviceIndependentSize) / static_cast<qreal>(nativeSize))
  );
}

///
///
auto MonitorMapping::horizontal(const std::int64_t nativePixels) const -> int
{
  return scaled(nativePixels, deviceIndependentGeometry.width(), bibstd::math::size(nativeGeometry.horizontal_range()));
}

///
///
auto MonitorMapping::vertical(const std::int64_t nativePixels) const -> int
{
  return scaled(nativePixels, deviceIndependentGeometry.height(), bibstd::math::size(nativeGeometry.vertical_range()));
}

///
///
auto MonitorMapping::map(const bibstd::util::screen_coordinates_type& position) const -> QPoint
{
  const auto origin = nativeGeometry.origin();
  return deviceIndependentGeometry.topLeft() +
         QPoint{horizontal(position.x() - origin.x()), vertical(position.y() - origin.y())};
}

///
/// Find the mapping of the monitor under the cursor. The native side looks the monitor up by the
/// native cursor, the QML layer the screen by its own cursor.
/// \return monitor mapping, or std::nullopt if no monitor or screen is under the cursor
///
[[nodiscard]] auto monitorMappingAt(const CursorPositionPair& cursor) -> std::optional<MonitorMapping>
{
  const auto monitor = bibstd::system::screen::monitor_at(cursor.native);
  const auto* const screen = QGuiApplication::screenAt(cursor.deviceIndependent);
  if(
    !monitor || screen == nullptr || screen->geometry().isEmpty() || bibstd::math::size(monitor->horizontal_range()) == 0u ||
    bibstd::math::size(monitor->vertical_range()) == 0u
  )
  {
    LOG_WARN("no monitor or screen found at cursor: cursor={}", cursor.native);
    return std::nullopt;
  }
  return MonitorMapping{.nativeGeometry = *monitor, .deviceIndependentGeometry = screen->geometry()};
}

} // namespace

///
///
auto toDeviceIndependent(const bibstd::util::screen_rect_type& rect, const CursorPositionPair& cursor) -> std::optional<QRect>
{
  const auto mapping = monitorMappingAt(cursor);
  if(!mapping)
  {
    return std::nullopt;
  }
  const auto size = QSize{
    mapping->horizontal(bibstd::math::size(rect.horizontal_range())),
    mapping->vertical(bibstd::math::size(rect.vertical_range()))
  };
  return QRect{mapping->map(rect.origin()), size};
}

} // namespace bibqml
