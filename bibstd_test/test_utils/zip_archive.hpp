#pragma once

#include <catch2/catch_test_macros.hpp>
#include <zip.h>

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace bibstd::io::test_utils
{

///
/// Entry of a zip archive: the name it is stored under and its content.
///
using zip_entry = std::pair<std::string, std::string>;

///
/// Write a zip archive holding the given entries, replacing one already at the path.
/// \note The entries have to outlive the call, their content is not copied.
///
inline auto write_zip_archive(const std::filesystem::path& path, const std::vector<zip_entry>& entries) -> void
{
  auto error = int{};
  auto* const archive = zip_open(path.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
  REQUIRE(archive != nullptr);
  for(const auto& [name, content] : entries)
  {
    auto* const data = zip_source_buffer(archive, content.data(), content.size(), 0);
    REQUIRE(data != nullptr);
    REQUIRE(zip_file_add(archive, name.c_str(), data, ZIP_FL_OVERWRITE) >= 0);
  }
  REQUIRE(zip_close(archive) == 0);
}

} // namespace bibstd::io::test_utils
