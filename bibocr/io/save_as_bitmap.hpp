#pragma once

#include "arcstd/data/pixel.hpp"
#include "arcstd/data/plane.hpp"

#include <filesystem>
#include <optional>

namespace bibocr::io
{

///
/// Save \p data, or its \p subarea, as bitmap file \p path, which should end with `.bmp`.
/// \return true if successful, false otherwise
///
[[nodiscard]] auto save_as_bitmap(
  const arcstd::data::plane_view<const arcstd::data::pixel>& data,
  std::optional<arcstd::data::plane_view<const arcstd::data::pixel>::area_type> subarea,
  const std::filesystem::path& path
) -> bool;

} // namespace bibocr::io
