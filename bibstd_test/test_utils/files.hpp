#pragma once

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace bibstd::test_utils
{

///
/// Write \p content to \p file, replacing it.
///
inline auto write_file(const std::filesystem::path& file, const std::string& content) -> void
{
  std::ofstream{file, std::ios::binary} << content;
}

///
/// \return the content of \p file, empty if it can not be read
///
[[nodiscard]] inline auto read_file(const std::filesystem::path& file) -> std::string
{
  auto stream = std::ifstream{file, std::ios::binary};
  return std::string{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

} // namespace bibstd::test_utils
