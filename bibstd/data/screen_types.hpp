#pragma once

#include "bibstd/data/pixel.hpp"
#include "bibstd/data/plane.hpp"
#include "bibstd/math/rect.hpp"

namespace bibstd::data
{

///
/// Types of the screen and of its pixels.
///
using screen_rect_type = math::rect<std::int32_t>;
using screen_coordinates_type = screen_rect_type::coordinates_type;
using pixel_plane_type = plane<pixel>;
using pixel_plane_view_type = plane_view<const pixel>;

} // namespace bibstd::data
