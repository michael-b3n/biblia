#pragma once

#include "bibstd/data/screen_types.hpp"
#include "bibstd/system/windows/win.hpp"

#include <d3d11.h>
#include <dxgi.h>
#include <inspectable.h>
#include <roapi.h>

#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>

#include <chrono>
#include <cstdint>
#include <format>
#include <memory>
#include <mutex>
#include <string>

namespace bibstd::system::winrt_capture
{

namespace capture_api = winrt::Windows::Graphics::Capture;
namespace directx_api = winrt::Windows::Graphics::DirectX;
namespace direct3d_api = winrt::Windows::Graphics::DirectX::Direct3D11;
namespace metadata_api = winrt::Windows::Foundation::Metadata;

// Hands out the graphics device texture behind a captured frame.
using dxgi_access_type = ::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess;

// Runtime class of the capture session, used to ask which of its properties this system has.
inline constexpr auto session_class = L"Windows.Graphics.Capture.GraphicsCaptureSession";

// How long a capture waits for the first frame of a session it just started.
inline constexpr auto first_frame_timeout = std::chrono::milliseconds{500};

// Frames the capture pool holds. Two suffice, only the newest frame is ever read.
inline constexpr std::int32_t frame_pool_size = 2;

// Bytes per pixel, frames are requested in a fixed bgra format.
inline constexpr std::uint32_t frame_pixel_size = 4;

///
/// Region of a monitor, in the coordinate system of that monitor.
///
struct region final
{
  std::int32_t x{0};
  std::int32_t y{0};
  std::int32_t width{0};
  std::int32_t height{0};
};

///
/// Initialize the calling thread for use of the capture API.
/// \note Which apartment the thread lands in does not matter, the frame pool is created free
/// threaded. The initialization is never taken back, there is no point at which a calling thread
/// is known to be done with the capture API.
///
inline auto init_thread() -> void
{
  static thread_local const auto initialized = []
  {
    const auto hr = ::RoInitialize(RO_INIT_MULTITHREADED);
    // Other code may initialize a thread itself, an already initialized thread is no error here
    return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
  }();
  static_cast<void>(initialized);
}

///
/// Format a windows error code the way the windows documentation writes them.
/// \return error code as hexadecimal string
///
inline auto to_string(const HRESULT hr) -> std::string
{
  return std::format("{:#010x}", static_cast<std::uint32_t>(hr));
}

///
/// Graphics device the capture runs on. It hands the capture API the device to deliver frames on
/// and it does the two copies a capture needs: the newest frame into a texture of our own, and the
/// wanted region of that texture into memory the cpu can read.
/// \note The immediate device context is not thread-safe, so every use of it is serialized here.
/// The frame callbacks of the capture API run on threads of their own, which makes that the point
/// of this class.
///
class device final
{
  // Variables
  const winrt::com_ptr<ID3D11Device> d3d_device_;
  const winrt::com_ptr<ID3D11DeviceContext> context_;
  const direct3d_api::IDirect3DDevice capture_device_;
  std::mutex mtx_;
  winrt::com_ptr<ID3D11Texture2D> staging_;
  std::int32_t staging_width_{0};
  std::int32_t staging_height_{0};

public: // Typedefs
  using pixel_plane_type = data::pixel_plane_type;
  using texture_type = winrt::com_ptr<ID3D11Texture2D>;

public: // Creators
  ///
  /// Create the graphics device.
  /// \return The created device, or nullptr if no device can be created
  ///
  static auto create() -> std::shared_ptr<device>;

public: // Constructor
  device(
    winrt::com_ptr<ID3D11Device> d3d_device,
    winrt::com_ptr<ID3D11DeviceContext> context,
    direct3d_api::IDirect3DDevice capture_device
  );
  ~device() noexcept;

  device(const device&) = delete;
  device(device&&) = delete;
  auto operator=(const device&) -> device& = delete;
  auto operator=(device&&) -> device& = delete;

public: // Accessors
  ///
  /// Access the device in the form the capture API takes it.
  /// \return capture device
  ///
  [[nodiscard]] auto capture_device() const -> const direct3d_api::IDirect3DDevice&;

public: // Modifiers
  ///
  /// Copy a frame into a texture of our own. A captured frame goes back to its pool as soon as the
  /// callback delivering it returns, its content has to be kept elsewhere to outlive that. The
  /// target texture is created on the first copy into it and whenever the frame size changes.
  /// \return true if the frame was copied, false otherwise
  ///
  auto keep_frame(const texture_type& frame, texture_type& target) -> bool;

  ///
  /// Read a region of a texture into pixels. The pixels are saved row by row, the topmost row of
  /// the region first. A region reaching past the texture is turned down.
  /// \return true if the region was read, false otherwise
  ///
  auto read_region(const texture_type& source, region reg, pixel_plane_type& pix) -> bool;

private: // Implementation
  ///
  /// Provide the texture the cpu reads a region of the given size out of, creating it if the
  /// cached one was made for a differently sized region.
  /// \return true if the texture is ready to be read into, false otherwise
  ///
  auto prepare_staging(std::int32_t width, std::int32_t height) -> bool;
};

} // namespace bibstd::system::winrt_capture
