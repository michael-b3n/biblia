#include "arcstd/system/screen.hpp"
#include "arcstd/util/numeric_cast.hpp"

#include "arcstd/system/windows/win.hpp"

namespace bibstd::system
{

///
///
auto screen::init() -> bool
{
  // Declare per-monitor DPI awareness for proper screen capture at native resolution.
  // This is also declared in the app manifest. The call here is a no-op if already set.
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  return true;
}

///
///
auto screen::metrics() -> screen_rect_type
{
  SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  const auto x = GetSystemMetrics(SM_XVIRTUALSCREEN);
  const auto y = GetSystemMetrics(SM_YVIRTUALSCREEN);
  const auto width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  const auto height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  return {
    math::coordinates{x, y},
    static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)
  };
}

///
///
auto screen::cursor_position() -> std::optional<screen_coordinates_type>
{
  POINT point{.x = 0, .y = 0};
  SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  if(!static_cast<bool>(GetCursorPos(&point)))
  {
    return std::nullopt;
  }
  return screen_coordinates_type{point.x, point.y};
}

///
///
auto screen::window_at(const screen_coordinates_type coordinates) -> std::optional<screen_rect_type>
{
  SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  HWND hwnd = WindowFromPoint(POINT{.x = coordinates.x(), .y = coordinates.y()});
  if(hwnd != nullptr)
  {
    RECT rect;
    if(static_cast<bool>(GetWindowRect(hwnd, &rect)))
    {
      return screen_rect_type(
        math::coordinates{numeric_cast<std::int32_t>(rect.left), numeric_cast<std::int32_t>(rect.bottom)},
        math::coordinates{numeric_cast<std::int32_t>(rect.right), numeric_cast<std::int32_t>(rect.top)}
      );
    }
  }
  return std::nullopt;
}

///
///
auto screen::monitor_at(const screen_coordinates_type coordinates) -> std::optional<screen_rect_type>
{
  SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  HMONITOR monitor = MonitorFromPoint(POINT{.x = coordinates.x(), .y = coordinates.y()}, MONITOR_DEFAULTTONULL);
  if(monitor == nullptr)
  {
    return std::nullopt;
  }
  MONITORINFO info;
  info.cbSize = sizeof(info);
  if(!static_cast<bool>(GetMonitorInfoW(monitor, &info)))
  {
    return std::nullopt;
  }
  decltype(auto) rect = info.rcMonitor;
  return screen_rect_type(
    math::coordinates{numeric_cast<std::int32_t>(rect.left), numeric_cast<std::int32_t>(rect.top)},
    math::coordinates{numeric_cast<std::int32_t>(rect.right), numeric_cast<std::int32_t>(rect.bottom)}
  );
}

} // namespace bibstd::system
