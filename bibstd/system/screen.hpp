#pragma once

#include "bibstd/data/screen_types.hpp"

#include <optional>

namespace bibstd::system
{

///
/// Screen capture class for windows implementation.
///
class screen final
{
public: // Typedefs
  using screen_rect_type = data::screen_rect_type;
  using screen_coordinates_type = data::screen_coordinates_type;
  using pixel_plane_type = data::pixel_plane_type;

public: // Static accessors
  ///
  /// Set screen related settings. This function must be called once before any other screen function is used.
  /// \return true if initialization was successful, false otherwise
  ///
  static auto init() -> bool;

  ///
  /// Get the virtual screen metrics.
  /// The metrics are given in the screen coordinate system, where the origin is on the top left corner.
  /// \return screen metrics
  ///
  [[nodiscard]] static auto metrics() -> screen_rect_type;

  ///
  /// Get the cursor position in virtual screen coordinate system.
  /// The cursor is givin in the screen coordinate system, where the origin is on the top left corner.
  /// \return screen metrics
  ///
  [[nodiscard]] static auto cursor_position() -> std::optional<screen_coordinates_type>;

  ///
  /// Get the window size at a given position. If no window is found, std::nullopt is returned.
  /// \return screen rectangle
  ///
  [[nodiscard]] static auto window_at(screen_coordinates_type coordinates) -> std::optional<screen_rect_type>;

  ///
  /// Get the monitor at a given position. If no monitor is found, std::nullopt is returned.
  /// The rectangle is given in the screen coordinate system, where the origin is on the top left corner.
  /// \return monitor rectangle
  ///
  [[nodiscard]] static auto monitor_at(screen_coordinates_type coordinates) -> std::optional<screen_rect_type>;

  ///
  /// Capture screen in region defined by a rectangle. The rectangle shall be in the
  /// screen coordinate system, where the origin is on the top left corner.
  /// A rectangle of screen area that shall be captured and the pixels object to save the
  /// captured pixels must be provided. The pixels are saved row by row, the topmost row of
  /// the region first (which is the order e.g. tesseract reads them in).
  ///
  [[nodiscard]] static auto capture(screen_rect_type rect, pixel_plane_type& pix) -> bool;
};

} // namespace bibstd::system
