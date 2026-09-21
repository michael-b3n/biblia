#include "bibstd/system/screen.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/non_owning_ptr.hpp"
#include "bibstd/util/numeric_cast.hpp"
#include "bibstd/util/ranges.hpp"

#include "bibstd/system/windows/screen_capture.hpp"
#include "bibstd/system/windows/win.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <mutex>
#include <ranges>

namespace bibstd::system
{

namespace
{

///
/// Capture a screen region through gdi. This is the fallback of the platform capture backend: it
/// works everywhere, but it reads back the region over the cpu and it sees hardware composited
/// content as black.
/// \return true if the region was captured, false otherwise
///
auto capture_gdi(const util::screen_rect_type rect, util::pixel_plane_type& pix) -> bool
{
  static std::mutex mtx;
  static std::vector<std::byte> pixels_bytes;

  const auto lock = std::scoped_lock{mtx};

  HDC hdc = GetDC(nullptr);
  HBITMAP bitmap = [&]
  {
    const auto hr = numeric_cast<int>(math::size(rect.horizontal_range()));
    const auto vr = numeric_cast<int>(math::size(rect.vertical_range()));

    HDC sdc = CreateCompatibleDC(hdc);
    auto* hbitmap = CreateCompatibleBitmap(hdc, hr, vr);
    HGDIOBJ old_bitmap = SelectObject(sdc, hbitmap);
    const auto origin = rect.origin();
    // No CAPTUREBLT: it makes the system hide and redraw the cursor, which blinks on every capture.
    // The cursor is never part of the screen DC anyway, so leaving the flag out costs nothing here.
    // NOLINTNEXTLINE(readability-suspicious-call-argument)
    BitBlt(sdc, 0, 0, hr, vr, hdc, origin.x(), origin.y(), SRCCOPY);
    SelectObject(sdc, old_bitmap);
    DeleteDC(sdc);
    return hbitmap;
  }();

  BITMAPINFO info = {};
  info.bmiHeader.biSize = sizeof(info.bmiHeader);
  if(0 == GetDIBits(hdc, bitmap, 0, 0, nullptr, &info, DIB_RGB_COLORS))
  {
    LOG_ERROR("capture screen failed: {}", "bitmap info not found");
    return false;
  }
  info.bmiHeader.biCompression = BI_RGB;
  if(pixels_bytes.size() < info.bmiHeader.biSizeImage)
  {
    pixels_bytes.resize(info.bmiHeader.biSizeImage);
  }

  if(0 == GetDIBits(hdc, bitmap, 0, info.bmiHeader.biHeight, static_cast<void*>(pixels_bytes.data()), &info, DIB_RGB_COLORS))
  {
    LOG_ERROR("capture screen failed: {}", "bitmap data not found");
    return false;
  }
  DeleteObject(bitmap);
  ReleaseDC(nullptr, hdc);
  assert(info.bmiHeader.biBitCount >= 24);

  // The loaded pixel bytes are saved to a list of pixels in a row reversed order.
  // This order makes the bitmap data directly compatible with tesseract.
  // Since the Windows screen coordinate system origin is on top left,
  // the highest row is the lowest row in the coordinate system of tesseract,
  // where the origin is on the bottom left.
  const auto byte_count = info.bmiHeader.biBitCount / 8;
  const auto height = numeric_cast<std::uint32_t>(info.bmiHeader.biHeight);
  const auto width = numeric_cast<std::uint32_t>(info.bmiHeader.biWidth);

  pix = util::pixel_plane_type(width, height);
  std::ranges::for_each(
    util::ranges::index_view_to(height) | std::views::reverse,
    [&, counter = 0u](const auto row_idx) mutable
    {
      std::ranges::for_each(
        util::ranges::index_view_between(width * row_idx, width * (row_idx + 1)),
        [&](const auto index)
        {
          const auto i = counter++ * byte_count;
          auto& p = pix.at(index);
          p.blue = static_cast<std::uint8_t>(pixels_bytes[i + 0]);
          p.green = static_cast<std::uint8_t>(pixels_bytes[i + 1]);
          p.red = static_cast<std::uint8_t>(pixels_bytes[i + 2]);
        }
      );
    }
  );
  return true;
}

///
/// Access the capture backend of this platform. The first call brings it up,
/// every later call hands out the same one. It is brought up by the thread that asks for the
/// first capture, so that no thread is left in a com apartment it did not ask to be in.
/// \return The backend, or nullptr if the platform offers none
///
auto capture_backend() -> util::non_owning_ptr<screen_capture>
{
  static const auto backend = []
  {
    auto created = screen_capture::create();
    LOG_INFO("screen capture backend: {}", created != nullptr ? "graphics capture" : "gdi");
    return created;
  }();
  return backend.get();
}

} // namespace

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

///
///
auto screen::capture(const screen_rect_type rect, pixel_plane_type& pix) -> bool
{
  // The backend turns a region down that it cannot serve, a region spanning two monitors for
  // instance, so gdi stays the answer for everything the newer API leaves out.
  const util::non_owning_ptr<screen_capture> backend = capture_backend();
  if(backend != nullptr && backend->capture(rect, pix))
  {
    return true;
  }
  return capture_gdi(rect, pix);
}

} // namespace bibstd::system
