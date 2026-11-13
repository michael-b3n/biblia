#include "bibocr/system/windows/capture_engine_winrt_device.hpp"

#include <arcstd/util/log.hpp>
#include <arcstd/util/numeric_cast.hpp>
#include <arcstd/util/ranges.hpp>
#include <arcstd/util/scope_guard.hpp>

#include <cstddef>
#include <utility>

namespace bibocr::system::winrt_capture
{
namespace
{

///
/// Create a d3d11 device and its immediate context.
/// \return true if the device was created, false otherwise
///
auto create_d3d_device(winrt::com_ptr<ID3D11Device>& d3d_device, winrt::com_ptr<ID3D11DeviceContext>& context) -> bool
{
  const auto create = [&d3d_device, &context](const D3D_DRIVER_TYPE driver)
  {
    d3d_device = nullptr;
    context = nullptr;
    return D3D11CreateDevice(
      nullptr,
      driver,
      nullptr,
      D3D11_CREATE_DEVICE_BGRA_SUPPORT,
      nullptr,
      0,
      D3D11_SDK_VERSION,
      d3d_device.put(),
      nullptr,
      context.put()
    );
  };

  auto hr = create(D3D_DRIVER_TYPE_HARDWARE);
  if(FAILED(hr))
  {
    // A system without a usable graphics adapter still captures, only on the cpu
    hr = create(D3D_DRIVER_TYPE_WARP);
  }
  if(FAILED(hr))
  {
    LOG_WARN("graphics device for screen capture not created: {}", to_string(hr));
    return false;
  }
  return true;
}

///
/// Read the size and format of a texture.
/// \return texture description
///
auto describe(const device::texture_type& texture) -> D3D11_TEXTURE2D_DESC
{
  D3D11_TEXTURE2D_DESC desc = {};
  texture->GetDesc(&desc);
  return desc;
}

} // namespace

///
///
auto device::create() -> std::shared_ptr<device>
{
  try
  {
    init_thread();
    auto d3d_device = winrt::com_ptr<ID3D11Device>{};
    auto context = winrt::com_ptr<ID3D11DeviceContext>{};
    if(!create_d3d_device(d3d_device, context))
    {
      return nullptr;
    }

    auto inspectable = winrt::com_ptr<::IInspectable>{};
    if(const auto hr = CreateDirect3D11DeviceFromDXGIDevice(d3d_device.as<IDXGIDevice>().get(), inspectable.put()); FAILED(hr))
    {
      LOG_WARN("graphics device not usable by the capture API: {}", to_string(hr));
      return nullptr;
    }
    return std::make_shared<device>(std::move(d3d_device), std::move(context), inspectable.as<direct3d_api::IDirect3DDevice>());
  }
  catch(const winrt::hresult_error& error)
  {
    LOG_WARN("graphics device for screen capture not created: {}", winrt::to_string(error.message()));
    return nullptr;
  }
}

///
///
device::device(
  winrt::com_ptr<ID3D11Device> d3d_device,
  winrt::com_ptr<ID3D11DeviceContext> context,
  direct3d_api::IDirect3DDevice capture_device
)
  : d3d_device_{std::move(d3d_device)}
  , context_{std::move(context)}
  , capture_device_{std::move(capture_device)}
{
}

///
///
device::~device() noexcept = default;

///
///
auto device::capture_device() const -> const direct3d_api::IDirect3DDevice&
{
  return capture_device_;
}

///
///
auto device::keep_frame(const texture_type& frame, texture_type& target) -> bool
{
  const auto lock = std::scoped_lock{mtx_};
  if(!frame)
  {
    return false;
  }
  auto desc = describe(frame);
  if(target)
  {
    // A monitor that changed its resolution delivers frames the kept texture no longer fits
    const auto kept = describe(target);
    if(kept.Width != desc.Width || kept.Height != desc.Height || kept.Format != desc.Format)
    {
      target = nullptr;
    }
  }
  if(!target)
  {
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    if(const auto hr = d3d_device_->CreateTexture2D(&desc, nullptr, target.put()); FAILED(hr))
    {
      LOG_WARN("frame texture not created: {}", to_string(hr));
      return false;
    }
  }
  context_->CopyResource(target.get(), frame.get());
  return true;
}

///
///
auto device::read_region(const texture_type& source, const region reg, pixel_plane_type& pix) -> bool
{
  const auto lock = std::scoped_lock{mtx_};
  if(!source || reg.x < 0 || reg.y < 0 || reg.width <= 0 || reg.height <= 0)
  {
    return false;
  }
  // The kept frame can be older than the region: a monitor that just changed resolution is
  // captured at its former size until the next frame arrives
  const auto desc = describe(source);
  if(numeric_cast<UINT>(reg.x + reg.width) > desc.Width || numeric_cast<UINT>(reg.y + reg.height) > desc.Height)
  {
    return false;
  }
  if(!prepare_staging(reg.width, reg.height))
  {
    return false;
  }

  const D3D11_BOX box = {
    .left = numeric_cast<UINT>(reg.x),
    .top = numeric_cast<UINT>(reg.y),
    .front = 0,
    .right = numeric_cast<UINT>(reg.x + reg.width),
    .bottom = numeric_cast<UINT>(reg.y + reg.height),
    .back = 1
  };
  context_->CopySubresourceRegion(staging_.get(), 0, 0, 0, 0, source.get(), 0, &box);
  pix = pixel_plane_type(numeric_cast<std::uint32_t>(reg.width), numeric_cast<std::uint32_t>(reg.height));

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  if(const auto hr = context_->Map(staging_.get(), 0, D3D11_MAP_READ, 0, &mapped); FAILED(hr))
  {
    LOG_ERROR("capture screen failed: {}", "captured region not readable");
    return false;
  }
  const auto unmap = arcstd::util::scope_guard{[this] { context_->Unmap(staging_.get(), 0); }};

  // Both the texture rows and the pixels run from the topmost row down, so the rows map straight
  // onto each other. Only the stride differs, the texture pads its rows.
  const auto width = numeric_cast<std::size_t>(reg.width);
  const auto row_pitch = numeric_cast<std::size_t>(mapped.RowPitch);
  for(const auto row : arcstd::util::ranges::index_view_to(numeric_cast<std::size_t>(reg.height)))
  {
    const auto* const source_row = static_cast<const std::uint8_t*>(mapped.pData) + (row * row_pitch);
    for(const auto column : arcstd::util::ranges::index_view_to(width))
    {
      const auto* const source_pixel = source_row + (column * frame_pixel_size);
      auto& p = pix.at((row * width) + column);
      p.blue = source_pixel[0];
      p.green = source_pixel[1];
      p.red = source_pixel[2];
    }
  }
  return true;
}

///
///
auto device::prepare_staging(const std::int32_t width, const std::int32_t height) -> bool
{
  if(staging_ && staging_width_ == width && staging_height_ == height)
  {
    return true;
  }
  staging_ = nullptr;
  staging_width_ = 0;
  staging_height_ = 0;

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = numeric_cast<UINT>(width);
  desc.Height = numeric_cast<UINT>(height);
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  if(const auto hr = d3d_device_->CreateTexture2D(&desc, nullptr, staging_.put()); FAILED(hr))
  {
    LOG_ERROR("capture screen failed: {}", "readable texture not created");
    return false;
  }
  staging_width_ = width;
  staging_height_ = height;
  return true;
}

} // namespace bibocr::system::winrt_capture
