#pragma once

#include <cstdint>

namespace bibstd::data
{

///
/// Represents a RGBA color. Each of these values can range from 0 to 255.
///
struct color_rgba final
{
  // Variables
  std::uint8_t red{0};
  std::uint8_t green{0};
  std::uint8_t blue{0};
  std::uint8_t alpha{0};

  // Operators
  auto operator==(const color_rgba&) const -> bool = default;
};

} // namespace bibstd::data
