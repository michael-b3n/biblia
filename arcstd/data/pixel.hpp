#pragma once

#include <cstdint>

namespace bibstd::data
{

///
/// Represents a single 32 bit pixel in the image. A pixel has red, green, blue and alpha
/// components that are mixed to form a color. Each of these values can range from 0 to 255.
///
struct pixel final
{
  // Constants
  static constexpr std::uint16_t bits_per_pixel = 32;
  static constexpr std::uint16_t bytes_per_pixel = bits_per_pixel / 8;

  // Variables
  std::uint8_t red{0};
  std::uint8_t green{0};
  std::uint8_t blue{0};
  std::uint8_t alpha{0};

  // Operators
  auto operator==(const pixel&) const -> bool = default;
};

} // namespace bibstd::data
