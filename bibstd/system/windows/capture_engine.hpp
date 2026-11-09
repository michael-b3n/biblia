#pragma once

#include "bibstd/data/screen_types.hpp"

#include <memory>

namespace bibstd::system
{

///
/// Interface of a screen capture backend. A backend may turn a region down, the caller then captures it otherwise.
/// \note Implementations shall be thread-safe.
///
class capture_engine
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
  [[nodiscard]] static auto create() -> std::unique_ptr<capture_engine>;

public: // Constructor
  capture_engine() = default;
  virtual ~capture_engine() noexcept;

  capture_engine(const capture_engine&) = delete;
  capture_engine(capture_engine&&) = delete;
  auto operator=(const capture_engine&) -> capture_engine& = delete;
  auto operator=(capture_engine&&) -> capture_engine& = delete;

public: // Modifiers
  ///
  /// Capture the screen region \p rect, origin top left, into \p pix row by row, the topmost row first.
  /// \return true if the region was captured, false if the caller shall capture it by other means
  ///
  [[nodiscard]] auto capture(screen_rect_type rect, pixel_plane_type& pix) -> bool;

private: // Implementation
  [[nodiscard]] virtual auto do_capture(screen_rect_type rect, pixel_plane_type& pix) -> bool = 0;
};

} // namespace bibstd::system
