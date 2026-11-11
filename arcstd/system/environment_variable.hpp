#pragma once

#include <string>

namespace arcstd::system
{

///
/// Environment variable class for generic OS.
///
struct environment_variable final
{
  ///
  /// Set an environment variable.
  /// \return true if the operation was successful, false otherwise
  ///
  [[nodiscard]] static auto set(const std::string& name, const std::string& value) -> bool;
};

} // namespace arcstd::system
