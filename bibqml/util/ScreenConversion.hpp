#pragma once

#include <bibstd/util/screen_types.hpp>

#include <QPoint>
#include <QRect>

#include <optional>

namespace bibqml
{

///
/// Cursor position read in native and device independent pixels at once. Both describe the same point,
/// which links the monitor under the cursor to the screen of the QML layer showing it.
///
struct CursorPositionPair final
{
  bibstd::util::screen_coordinates_type native;
  QPoint deviceIndependent;
};

///
/// Map a rectangle in native pixels of the virtual screen to the device independent pixels the QML
/// layer positions its windows in. Qt scales every monitor on its own, the rectangle is mapped by its
/// relative placement on the monitor under the cursor.
/// \note This accesses the screens of the QML layer, it must be called on its thread.
/// \return rectangle in device independent pixels, std::nullopt if no monitor or screen is under the cursor
///
[[nodiscard]] auto toDeviceIndependent(const bibstd::util::screen_rect_type& rect, const CursorPositionPair& cursor)
  -> std::optional<QRect>;

} // namespace bibqml
