#pragma once

#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"
#include "test_utils/zip_archive.hpp"

#include <bibstd/bible/common.hpp>
#include <bibstd/bible/reference.hpp>
#include <bibstd/bible/scripture.hpp>
#include <bibstd/workflow/workflow_script.hpp>
#include <bibstd/workflow/workflow_scripture_store.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::test_utils
{

///
/// The USFM codes of the books of the app, in their order.
///
inline constexpr auto usfm_codes = std::array<std::string_view, 66>{
  "GEN", "EXO", "LEV", "NUM", "DEU", "JOS", "JDG", "RUT", "1SA", "2SA", "1KI", "2KI", "1CH", "2CH", "EZR", "NEH", "EST",
  "JOB", "PSA", "PRO", "ECC", "SNG", "ISA", "JER", "LAM", "EZK", "DAN", "HOS", "JOL", "AMO", "OBA", "JON", "MIC", "NAM",
  "HAB", "ZEP", "HAG", "ZEC", "MAL", "MAT", "MRK", "LUK", "JHN", "ACT", "ROM", "1CO", "2CO", "GAL", "EPH", "PHP", "COL",
  "1TH", "2TH", "1TI", "2TI", "TIT", "PHM", "HEB", "JAS", "1PE", "2PE", "1JN", "2JN", "3JN", "JUD", "REV",
};

///
/// \return the entries of a zip archive of USFM files: a file per book
/// with the verse "<code> 1:1", or the content \p books has for its code
///
[[nodiscard]] inline auto usfm_entries(const std::map<std::string, std::string>& books = {})
  -> std::vector<io::test_utils::zip_entry>
{
  auto entries = std::vector<io::test_utils::zip_entry>{};
  for(const auto code : usfm_codes)
  {
    const auto it = books.find(std::string{code});
    entries.emplace_back(
      std::format("{}.usfm", code), it != books.cend() ? it->second : std::format("\\id {0}\n\\c 1\n\\p\n\\v 1 {0} 1:1\n", code)
    );
  }
  return entries;
}

///
/// Scripture store with the bundled readers, loaded once the scripts are loaded. Its
/// folder holds \p archives as zip archives by file name and copies of \p files.
///
struct scripture_store final
{
  temp_folder folder;
  std::shared_ptr<workflow::workflow_script> script;
  std::shared_ptr<workflow::workflow_scripture_store> store;

  explicit scripture_store(
    std::string_view name,
    const std::map<std::string, std::vector<io::test_utils::zip_entry>>& archives = {},
    const std::vector<std::filesystem::path>& files = {}
  );

  ///
  /// \return the scripture folder of the store
  ///
  [[nodiscard]] auto scripture_folder() const -> std::filesystem::path;
};

///
///
inline scripture_store::scripture_store(
  const std::string_view name,
  const std::map<std::string, std::vector<io::test_utils::zip_entry>>& archives,
  const std::vector<std::filesystem::path>& files
)
  : folder{name}
{
  std::filesystem::create_directories(scripture_folder());
  for(const auto& [file_name, entries] : archives)
  {
    io::test_utils::write_zip_archive(scripture_folder() / file_name, entries);
  }
  for(const auto& file : files)
  {
    std::filesystem::copy_file(file, scripture_folder() / file.filename());
  }
  const auto settings = make_script_settings(folder.path());
  script = std::make_shared<workflow::workflow_script>(settings);
  store = std::make_shared<workflow::workflow_scripture_store>(settings, script);
  load_scripts(*script);
  store->load();
}

///
///
inline auto scripture_store::scripture_folder() const -> std::filesystem::path
{
  return folder.path() / workflow::workflow_scripture_store::folder_name;
}

///
/// \return the scripture the bundled readers read from a zip archive of \p entries, nullptr if none
///
[[nodiscard]] inline auto read_scripture(const std::string_view name, const std::vector<io::test_utils::zip_entry>& entries)
  -> std::shared_ptr<const bible::scripture>
{
  const auto fixture = scripture_store{name, {{"scripture.zip", entries}}};
  const auto scriptures = fixture.store->scriptures();
  return scriptures.empty() ? nullptr : scriptures.begin()->second;
}

///
/// \return the markup of the verse \p verse of \p chapter of \p book in \p scripture, none if it has none
///
[[nodiscard]] inline auto verse_of(
  const bible::scripture& scripture, const bible::book_id book, const std::uint32_t chapter, const std::uint32_t verse
) -> std::optional<std::string>
{
  const auto passage = scripture.passage(bible::reference::create_unguarded(book, chapter, verse));
  return passage ? std::optional{passage->content} : std::nullopt;
}

} // namespace bibstd::test_utils
