#include "bibstd/system/windows/screen_capture_winrt.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/numeric_cast.hpp"

#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>

#include <chrono>
#include <condition_variable>
#include <optional>
#include <utility>
#include <vector>

namespace bibstd::system
{
namespace winrt_capture
{

///
/// Standing capture of a single monitor. The session runs from construction until destruction and
/// keeps the newest frame it delivered around, so reading a region costs a copy of that region
/// instead of a capture of the whole monitor.
/// \note A monitor that cannot be captured is a monitor whose regions are all turned down, the
/// caller does not have to tell the two apart.
///
class monitor final
{
public: // Typedefs
  using pixel_plane_type = data::pixel_plane_type;

public: // Constructor
  ///
  /// Start capturing the given monitor. The size shall be the size of the monitor in the screen
  /// metrics, a capture that sizes it differently is turned down: regions are addressed in the
  /// coordinate system of the monitor and that mapping would not hold.
  ///
  monitor(std::shared_ptr<device> graphics, HMONITOR handle, std::int32_t width, std::int32_t height);
  ~monitor() noexcept;

  monitor(const monitor&) = delete;
  monitor(monitor&&) = delete;
  auto operator=(const monitor&) -> monitor& = delete;
  auto operator=(monitor&&) -> monitor& = delete;

public: // Accessors
  ///
  /// Access the monitor size this capture was set up for.
  /// \return size in pixels
  ///
  [[nodiscard]] auto width() const -> std::int32_t;
  [[nodiscard]] auto height() const -> std::int32_t;

public: // Modifiers
  ///
  /// Read a region of the newest captured frame. The first read of a session waits for its first
  /// frame, every later read is served right away.
  /// \return true if the region was read, false if the caller shall capture it by other means
  ///
  [[nodiscard]] auto read_region(region reg, pixel_plane_type& pix) -> bool;

private: // Implementation
  ///
  /// Everything the frame callback touches, kept behind a shared_ptr rather than in the monitor:
  /// taking the handler off does not wait for a callback that is already running, so the state a
  /// callback works on has to outlive the monitor for as long as the pool holds the handler.
  ///
  struct state final
  {
    explicit state(std::shared_ptr<device> graphics_device);

    const std::shared_ptr<device> graphics;
    std::mutex mtx;
    std::condition_variable frame_arrived;
    device::texture_type frame;
    bool has_frame{false};
    bool capturing{false};
  };

  ///
  /// Set the capture session of the monitor up and start it.
  /// \return true if the session is running, false otherwise
  ///
  auto open(HMONITOR handle) -> bool;

  ///
  /// Keep the newest frame the session delivered. Runs on a thread of the capture API.
  ///
  static auto on_frame_arrived(state& shared, const capture_api::Direct3D11CaptureFramePool& sender) -> void;

private: // Variables
  const std::shared_ptr<state> state_;
  const std::int32_t width_;
  const std::int32_t height_;
  capture_api::Direct3D11CaptureFramePool pool_{nullptr};
  capture_api::GraphicsCaptureSession session_{nullptr};
  winrt::event_token frame_token_{};
};

///
///
monitor::state::state(std::shared_ptr<device> graphics_device)
  : graphics{std::move(graphics_device)}
{
}

///
///
monitor::monitor(std::shared_ptr<device> graphics, HMONITOR handle, const std::int32_t width, const std::int32_t height)
  : state_{std::make_shared<state>(std::move(graphics))}
  , width_{width}
  , height_{height}
{
  try
  {
    // No lock, nothing else can reach this monitor yet and the callback never reads the flag
    state_->capturing = open(handle);
  }
  catch(const winrt::hresult_error& error)
  {
    LOG_WARN("monitor capture not started: {}", winrt::to_string(error.message()));
  }
}

///
///
monitor::~monitor() noexcept
{
  try
  {
    // A frame callback may still be running here, it works on the shared state and not on members
    if(pool_ && frame_token_)
    {
      pool_.FrameArrived(frame_token_);
    }
    if(session_)
    {
      session_.Close();
    }
    if(pool_)
    {
      pool_.Close();
    }
  }
  catch(...)
  {
    LOG_WARN("monitor capture not stopped: {}", util::exception_report());
  }
}

///
///
auto monitor::width() const -> std::int32_t
{
  return width_;
}

///
///
auto monitor::height() const -> std::int32_t
{
  return height_;
}

///
///
auto monitor::read_region(const region reg, pixel_plane_type& pix) -> bool
{
  auto lock = std::unique_lock{state_->mtx};
  if(!state_->capturing)
  {
    return false;
  }
  if(!state_->frame_arrived.wait_for(lock, first_frame_timeout, [this] { return state_->has_frame; }))
  {
    // Only the very first frame is ever waited for, so a timeout means this session never
    // produces anything. Give it up instead of paying the timeout on every capture.
    LOG_WARN("windows graphics capture delivered no frame, capturing this monitor by other means");
    state_->capturing = false;
    return false;
  }
  return state_->graphics->read_region(state_->frame, reg, pix);
}

///
///
auto monitor::open(HMONITOR handle) -> bool
{
  auto item = capture_api::GraphicsCaptureItem{nullptr};
  const auto interop = winrt::get_activation_factory<capture_api::GraphicsCaptureItem>().as<IGraphicsCaptureItemInterop>();
  const auto hr = interop->CreateForMonitor(handle, winrt::guid_of<capture_api::GraphicsCaptureItem>(), winrt::put_abi(item));
  if(FAILED(hr) || !item)
  {
    LOG_WARN("capture item for monitor not created: {}", to_string(hr));
    return false;
  }

  const auto size = item.Size();
  if(size.Width != width_ || size.Height != height_)
  {
    LOG_WARN("capture item size {}x{} does not match monitor size {}x{}", size.Width, size.Height, width_, height_);
    return false;
  }

  pool_ = capture_api::Direct3D11CaptureFramePool::CreateFreeThreaded(
    state_->graphics->capture_device(), directx_api::DirectXPixelFormat::B8G8R8A8UIntNormalized, frame_pool_size, size
  );
  session_ = pool_.CreateCaptureSession(item);
  session_.IsCursorCaptureEnabled(false);
  // The capture border around the monitor can only be turned off from a later windows version on
  if(metadata_api::ApiInformation::IsPropertyPresent(session_class, L"IsBorderRequired"))
  {
    session_.IsBorderRequired(false);
  }

  frame_token_ = pool_.FrameArrived(
    [shared = state_](const capture_api::Direct3D11CaptureFramePool& sender, const winrt::Windows::Foundation::IInspectable&)
    { on_frame_arrived(*shared, sender); }
  );
  session_.StartCapture();
  return true;
}

///
///
auto monitor::on_frame_arrived(state& shared, const capture_api::Direct3D11CaptureFramePool& sender) -> void
{
  try
  {
    const auto frame = sender.TryGetNextFrame();
    if(!frame)
    {
      return;
    }
    auto texture = device::texture_type{};
    frame.Surface().as<dxgi_access_type>()->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void());
    if(!texture)
    {
      return;
    }

    const auto lock = std::scoped_lock{shared.mtx};
    if(shared.graphics->keep_frame(texture, shared.frame))
    {
      shared.has_frame = true;
      shared.frame_arrived.notify_all();
    }
  }
  catch(const winrt::hresult_error& error)
  {
    LOG_WARN("capture frame not received: {}", winrt::to_string(error.message()));
  }
}

} // namespace winrt_capture

namespace
{

///
/// A screen region as the capture API addresses it: the monitor showing it, the size that monitor
/// has according to the screen metrics, and the region within that monitor.
///
struct placement final
{
  HMONITOR handle{nullptr};
  std::int32_t monitor_width{0};
  std::int32_t monitor_height{0};
  winrt_capture::region region;
};

///
/// Locate a screen region on the monitor showing it.
/// \return Placement of the region, or std::nullopt if no single monitor shows all of it
///
auto place(const data::screen_rect_type rect) -> std::optional<placement>
{
  const auto width = numeric_cast<std::int32_t>(math::size(rect.horizontal_range()));
  const auto height = numeric_cast<std::int32_t>(math::size(rect.vertical_range()));
  if(width == 0 || height == 0)
  {
    return std::nullopt;
  }

  const auto center = rect.center();
  auto* const handle = MonitorFromPoint(POINT{.x = center.x(), .y = center.y()}, MONITOR_DEFAULTTONULL);
  if(handle == nullptr)
  {
    return std::nullopt;
  }
  MONITORINFO info = {.cbSize = sizeof(MONITORINFO)};
  if(GetMonitorInfoW(handle, &info) == 0)
  {
    return std::nullopt;
  }

  // A capture covers exactly one monitor, a region reaching over a monitor edge is not ours
  const auto origin = rect.origin();
  const auto& bounds = info.rcMonitor;
  if(
    origin.x() < bounds.left || origin.y() < bounds.top || origin.x() + width > bounds.right ||
    origin.y() + height > bounds.bottom
  )
  {
    return std::nullopt;
  }
  return placement{
    .handle = handle,
    .monitor_width = numeric_cast<std::int32_t>(bounds.right - bounds.left),
    .monitor_height = numeric_cast<std::int32_t>(bounds.bottom - bounds.top),
    .region = {.x = origin.x() - bounds.left, .y = origin.y() - bounds.top, .width = width, .height = height}
  };
}

///
/// Request captures without the colored border, windows ignores IsBorderRequired(false) until then.
/// A packaged app asks the user once, an unpackaged one is granted without a prompt.
/// Captures run either way, without the access they show the border.
///
auto request_borderless_capture() -> void
{
  // How long the startup waits for the user to answer the prompt for captures without a border
  static constexpr auto borderless_consent_timeout = std::chrono::minutes{1};
  try
  {
    const auto request = winrt_capture::capture_api::GraphicsCaptureAccess::RequestAccessAsync(
      winrt_capture::capture_api::GraphicsCaptureAccessKind::Borderless
    );
    if(request.wait_for(borderless_consent_timeout) != winrt::Windows::Foundation::AsyncStatus::Completed)
    {
      LOG_WARN("borderless capture not answered in time, captures show the border until the next start");
      return;
    }
    LOG_INFO("borderless capture access: {}", util::enum_name(request.GetResults()));
  }
  catch(...)
  {
    LOG_WARN("borderless capture access not requested: {}", util::exception_report());
  }
}

} // namespace

///
///
auto screen_capture::create() -> std::unique_ptr<screen_capture>
{
  try
  {
    winrt_capture::init_thread();
    if(!winrt_capture::capture_api::GraphicsCaptureSession::IsSupported())
    {
      LOG_INFO("windows graphics capture is not supported on this system");
      return nullptr;
    }
    // Without this property a capture draws the cursor into every frame, which is the one thing it
    // must not do. Rather capture by other means than capture a cursor.
    if(!winrt_capture::metadata_api::ApiInformation::IsPropertyPresent(winrt_capture::session_class, L"IsCursorCaptureEnabled"))
    {
      LOG_INFO("windows graphics capture cannot exclude the cursor on this system");
      return nullptr;
    }
    // Asked before the first session starts, so every session starts with the answer
    if(winrt_capture::metadata_api::ApiInformation::IsPropertyPresent(winrt_capture::session_class, L"IsBorderRequired"))
    {
      request_borderless_capture();
    }
    auto capture_device = winrt_capture::device::create();
    if(!capture_device)
    {
      return nullptr;
    }
    return std::make_unique<screen_capture_winrt>(std::move(capture_device));
  }
  catch(const winrt::hresult_error& error)
  {
    LOG_WARN("windows graphics capture setup failed: {}", winrt::to_string(error.message()));
    return nullptr;
  }
}

///
///
screen_capture_winrt::screen_capture_winrt(std::shared_ptr<winrt_capture::device> capture_device)
  : device_{std::move(capture_device)}
{
}

///
///
screen_capture_winrt::~screen_capture_winrt() noexcept = default;

///
///
auto screen_capture_winrt::do_capture(const screen_rect_type rect, pixel_plane_type& pix) -> bool
{
  try
  {
    winrt_capture::init_thread();
    const auto placement = place(rect);
    if(!placement)
    {
      return false;
    }
    const auto current = monitor_of(placement->handle, placement->monitor_width, placement->monitor_height);
    return current->read_region(placement->region, pix);
  }
  catch(...)
  {
    LOG_WARN("windows graphics capture failed: {}", util::exception_report());
    return false;
  }
}

///
///
auto screen_capture_winrt::monitor_of(HMONITOR handle, const std::int32_t width, const std::int32_t height)
  -> std::shared_ptr<winrt_capture::monitor>
{
  // Declared before the lock and therefore given up after it: giving a capture up waits for a
  // frame callback that may still be running, and that callback takes locks of its own
  auto stale = std::vector<std::shared_ptr<winrt_capture::monitor>>{};
  const auto lock = std::scoped_lock{mtx_};

  // Ask whether a monitor handle still names a monitor of this system.
  static constexpr auto is_attached = [](HMONITOR handle) -> bool
  {
    MONITORINFO info = {.cbSize = sizeof(MONITORINFO)};
    return GetMonitorInfoW(handle, &info) != 0;
  };

  // A monitor that was unplugged would otherwise keep its session running for the whole process
  std::erase_if(
    monitors_,
    [&stale](auto& entry)
    {
      if(is_attached(entry.first))
      {
        return false;
      }
      stale.push_back(std::move(entry.second));
      return true;
    }
  );

  if(const auto found = monitors_.find(handle); found != monitors_.end())
  {
    if(found->second->width() == width && found->second->height() == height)
    {
      return found->second;
    }
    // The monitor changed its resolution, the running capture no longer fits it
    stale.push_back(std::move(found->second));
    monitors_.erase(found);
  }
  auto started = std::make_shared<winrt_capture::monitor>(device_, handle, width, height);
  return monitors_.emplace(handle, std::move(started)).first->second;
}

} // namespace bibstd::system
