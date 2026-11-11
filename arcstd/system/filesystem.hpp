#pragma once

#include <boost/dll.hpp>
#include <filesystem>
#include <optional>
#include <string_view>

namespace arcstd::system
{

///
/// Filesystem class for windows implementation.
///
struct filesystem final
{
  ///
  /// Get path to executable.
  /// \return path to executable location
  ///
  [[nodiscard]] static inline auto executable_location() -> std::filesystem::path;

  ///
  /// Get path to executable folder.
  /// \return path to executable folder
  ///
  [[nodiscard]] static inline auto executable_folder() -> std::filesystem::path;

  ///
  /// Get path to the local data folder \p folder_name, named after the executable if not set.
  /// \return path to local data
  ///
  [[nodiscard]] static auto local_data_folder(std::optional<std::string_view> folder_name = std::nullopt)
    -> std::filesystem::path;
};

///
///
inline auto filesystem::executable_location() -> std::filesystem::path
{
  return boost::dll::program_location().string();
}

///
///
inline auto filesystem::executable_folder() -> std::filesystem::path
{
  return boost::dll::program_location().parent_path().string();
}

} // namespace arcstd::system
