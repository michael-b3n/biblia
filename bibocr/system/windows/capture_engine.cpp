#include "bibocr/system/windows/capture_engine.hpp"

namespace bibstd::system
{

///
///
capture_engine::~capture_engine() noexcept = default;

///
///
auto capture_engine::capture(const screen_rect_type rect, pixel_plane_type& pix) -> bool
{
  return do_capture(rect, pix);
}

} // namespace bibstd::system
