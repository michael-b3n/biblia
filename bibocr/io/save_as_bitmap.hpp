#pragma once

#include "bibocr/data/pixel.hpp"
#include "bibocr/data/plane.hpp"

#include <filesystem>
#include <optional>

namespace bibocr::io
{

///
/// Save \p data, or its \p subarea, as bitmap file \p path, which should end with `.bmp`.
/// \return true if successful, false otherwise
///
[[nodiscard]] auto save_as_bitmap(
  const bibocr::data::plane_view<const bibocr::data::pixel>& data,
  std::optional<bibocr::data::plane_view<const bibocr::data::pixel>::area_type> subarea,
  const std::filesystem::path& path
) -> bool;

} // namespace bibocr::io
