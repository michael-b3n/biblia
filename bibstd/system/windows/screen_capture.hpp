#pragma once

#include "bibstd/data/screen_types.hpp"

#include <memory>

namespace bibstd::system
{

///
/// Virtual base class for the screen capture backend.
/// This class provides an interface for capturing a region of the screen through whichever
/// capture facility the platform offers. A backend may turn a region down that it cannot serve,
/// the caller is then expected to capture that region by other means.
/// \note The implementation of this interface shall be thread-safe.
///
class screen_capture
{
public: // Typedefs
  using screen_rect_type = data::screen_rect_type;
  using pixel_plane_type = data::pixel_plane_type;

public: // Creators
  ///
  /// Create the capture backend of this platform.
  /// \return The created backend, or nullptr if the platform offers none. A nullptr is final,
  /// nothing will start working later on.
  ///
  static auto create() -> std::unique_ptr<screen_capture>;

public: // Constructor
  screen_capture() = default;
  virtual ~screen_capture() noexcept;

  screen_capture(const screen_capture&) = delete;
  screen_capture(screen_capture&&) = delete;
  auto operator=(const screen_capture&) -> screen_capture& = delete;
  auto operator=(screen_capture&&) -> screen_capture& = delete;

public: // Modifiers
  ///
  /// Capture the screen region described by the given rectangle. The rectangle shall be in the
  /// screen coordinate system, where the origin is on the top left corner. The pixels are saved
  /// row by row, the topmost row of the region first.
  /// \return true if the region was captured, false if the caller shall capture it by other means
  ///
  [[nodiscard]] auto capture(screen_rect_type rect, pixel_plane_type& pix) -> bool;

private: // Implementation
  virtual auto do_capture(screen_rect_type rect, pixel_plane_type& pix) -> bool = 0;
};

} // namespace bibstd::system
