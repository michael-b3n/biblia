#pragma once

#include "bibstd/system/windows/screen_capture.hpp"
#include "bibstd/system/windows/screen_capture_winrt_device.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

// Forward declarations
namespace bibstd::system::winrt_capture
{
class monitor;
} // namespace bibstd::system::winrt_capture
namespace bibstd::system
{

///
/// Screen capture backend built on the Windows.Graphics.Capture API.
/// One capture is kept running per monitor that was asked for so far and every capture is served
/// from the newest frame that capture delivered, so a capture costs a copy of the wanted region on
/// the graphics device instead of a read of the whole desktop. The backend also sees hardware
/// composited content, which the gdi path hands back as black, and it excludes the cursor by
/// contract rather than by the cursor happening not to be part of a device context.
///
class screen_capture_winrt final : public screen_capture
{
  // Variables
  const std::shared_ptr<winrt_capture::device> device_;
  std::mutex mtx_;
  std::unordered_map<HMONITOR, std::shared_ptr<winrt_capture::monitor>> monitors_;

public: // Constructor
  explicit screen_capture_winrt(std::shared_ptr<winrt_capture::device> capture_device);
  ~screen_capture_winrt() noexcept override;

  screen_capture_winrt(const screen_capture_winrt&) = delete;
  screen_capture_winrt(screen_capture_winrt&&) = delete;
  auto operator=(const screen_capture_winrt&) -> screen_capture_winrt& = delete;
  auto operator=(screen_capture_winrt&&) -> screen_capture_winrt& = delete;

private: // Overrides
  ///
  /// A region reaching over a monitor edge is turned down, a capture covers exactly one monitor.
  /// \see screen_capture::capture
  ///
  auto do_capture(screen_rect_type rect, pixel_plane_type& pix) -> bool override;

private: // Implementation
  ///
  /// Provide the running capture of a monitor, starting one if the monitor is not captured yet or
  /// if its resolution changed since the capture was started. Captures of monitors that are gone
  /// are given up on the way.
  /// \return capture of the monitor
  ///
  auto monitor_of(HMONITOR handle, std::int32_t width, std::int32_t height) -> std::shared_ptr<winrt_capture::monitor>;
};

} // namespace bibstd::system
