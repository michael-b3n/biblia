#pragma once

#include "bibstd/data/pixel.hpp"
#include "bibstd/data/plane.hpp"

#include <filesystem>
#include <optional>

namespace bibstd::io
{

///
/// Save \p data, or its \p subarea, as bitmap file \p path, which should end with `.bmp`.
/// \return true if successful, false otherwise
///
auto save_as_bitmap(
  const data::plane_view<const data::pixel>& data,
  std::optional<data::plane_view<const data::pixel>::area_type> subarea,
  const std::filesystem::path& path
) -> bool;

} // namespace bibstd::io
