#pragma once

#include "bibocr/data/pixel.hpp"
#include "bibocr/data/plane.hpp"

#include <arcstd/math/rect.hpp>

namespace bibocr::data
{

///
/// Types of the screen and of its pixels.
///
using screen_rect_type = arcstd::math::rect<std::int32_t>;
using screen_coordinates_type = screen_rect_type::coordinates_type;
using pixel_plane_type = plane<pixel>;
using pixel_plane_view_type = plane_view<const pixel>;

} // namespace bibocr::data
