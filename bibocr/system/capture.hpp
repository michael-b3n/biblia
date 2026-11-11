#pragma once

#include "arcstd/data/screen_types.hpp"

namespace bibstd::system
{

///
/// Screen capture class for windows implementation.
///
class capture final
{
public: // Typedefs
  using screen_rect_type = data::screen_rect_type;
  using pixel_plane_type = data::pixel_plane_type;

public: // Static accessors
  ///
  /// Capture the screen region \p rect, origin top left, into \p pix row by row, the topmost row first as tesseract
  /// reads them.
  /// \return true if the region was captured
  ///
  [[nodiscard]] static auto region(screen_rect_type rect, pixel_plane_type& pix) -> bool;
};

} // namespace bibstd::system
