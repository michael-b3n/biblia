#pragma once

#include "bibocr/data/screen_types.hpp"

namespace bibocr::system
{

///
/// Screen capture class for windows implementation.
///
class capture final
{
public: // Typedefs
  using screen_rect_type = bibocr::data::screen_rect_type;
  using pixel_plane_type = bibocr::data::pixel_plane_type;

public: // Static accessors
  ///
  /// Capture the bibstd::system::screen region \p rect, origin top left, into \p pix row by row, the topmost row first as
  /// tesseract reads them.
  /// \return true if the region was captured
  ///
  [[nodiscard]] static auto region(screen_rect_type rect, pixel_plane_type& pix) -> bool;
};

} // namespace bibocr::system
