#pragma once

#include "bibstd/system/windows/capture_engine.hpp"
#include "bibstd/system/windows/capture_engine_winrt_device.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace bibstd::system::winrt_capture
{
class monitor;
} // namespace bibstd::system::winrt_capture

namespace bibstd::system
{

///
/// Screen capture backend on Windows.Graphics.Capture, one running capture per monitor asked for. A region costs a
/// copy on the graphics device, and unlike gdi it sees composited content and leaves out the cursor.
///
class capture_engine_winrt final : public capture_engine
{
  // Variables
  const std::shared_ptr<winrt_capture::device> device_;
  std::mutex mtx_;
  std::unordered_map<HMONITOR, std::shared_ptr<winrt_capture::monitor>> monitors_;

public: // Constructor
  explicit capture_engine_winrt(std::shared_ptr<winrt_capture::device> capture_device);
  ~capture_engine_winrt() noexcept override;

  capture_engine_winrt(const capture_engine_winrt&) = delete;
  capture_engine_winrt(capture_engine_winrt&&) = delete;
  auto operator=(const capture_engine_winrt&) -> capture_engine_winrt& = delete;
  auto operator=(capture_engine_winrt&&) -> capture_engine_winrt& = delete;

private: // Overrides
  ///
  /// A region reaching over a monitor edge is turned down, a capture covers exactly one monitor.
  /// \see capture_engine::capture
  ///
  [[nodiscard]] auto do_capture(screen_rect_type rect, pixel_plane_type& pix) -> bool override;

private: // Implementation
  ///
  /// Provide the running capture of a monitor, starting one for a new monitor or resolution and giving up those of
  /// monitors that are gone.
  /// \return capture of the monitor
  ///
  [[nodiscard]] auto monitor_of(HMONITOR handle, std::int32_t width, std::int32_t height) -> std::shared_ptr<winrt_capture::monitor>;
};

} // namespace bibstd::system
