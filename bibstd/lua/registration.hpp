#pragma once

#include "bibstd/util/path.hpp"

#include <memory>
#include <vector>

namespace bibstd::lua
{
// Forward declarations
class state;

namespace detail
{
struct state_data;
} // namespace detail

///
/// Registration of functions and settings for scripts, kept by who registered them, \see state::register_function.
/// Destroyed, it takes back what it registered and nothing else: the paths are free again and a script still holding
/// a function gets an error calling it. It waits for the running script, which may be inside a function.
///
class registration final
{
  // Friends
  friend class state;

  // Typedefs
  struct entry final
  {
    std::weak_ptr<detail::state_data> data;
    util::path path;
    std::shared_ptr<const void> registered;
  };

  // Variables
  std::vector<entry> entries_;

public: // Structors
  registration() = default;
  registration(const registration&) = delete;
  registration(registration&& other) noexcept;
  ~registration() noexcept;

public: // Operators
  auto operator=(const registration&) -> registration& = delete;
  auto operator=(registration&& other) noexcept -> registration&;

  ///
  /// Take over what \p other registered, so one registration holds all of an object:
  /// registration << register_function(...) << register_function(...).
  /// \return this registration
  ///
  auto operator<<(registration other) -> registration&;

  ///
  /// \return true if something is registered
  ///
  explicit operator bool() const noexcept;

private: // Structors
  registration(std::weak_ptr<detail::state_data> data, util::path path, std::shared_ptr<const void> registered);

private: // Implementation
  auto release() noexcept -> void;
};

} // namespace bibstd::lua
