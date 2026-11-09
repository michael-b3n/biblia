#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

namespace bibstd::test_utils
{

///
/// Empty folder in the temp directory, removed again with this object.
/// Named by the test, so tests running at the same time never share one.
///
class temp_folder final
{
  // Variables
  const std::filesystem::path path_;

public: // Structors
  explicit temp_folder(std::string_view name);
  ~temp_folder() noexcept;

  temp_folder(const temp_folder&) = delete;
  auto operator=(const temp_folder&) -> temp_folder& = delete;

public: // Accessors
  ///
  /// \return the path of the folder
  ///
  [[nodiscard]] auto path() const -> const std::filesystem::path&;
};

///
///
inline temp_folder::temp_folder(const std::string_view name)
  : path_{std::filesystem::temp_directory_path() / "bibstd_test" / name}
{
  std::filesystem::remove_all(path_);
  std::filesystem::create_directories(path_);
}

///
///
inline temp_folder::~temp_folder() noexcept
{
  auto error = std::error_code{};
  std::filesystem::remove_all(path_, error);
}

///
///
inline auto temp_folder::path() const -> const std::filesystem::path&
{
  return path_;
}

} // namespace bibstd::test_utils
