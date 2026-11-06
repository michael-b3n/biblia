#pragma once

#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"
#include "test_utils/zip_archive.hpp"

#include <bibstd/bible/common.hpp>
#include <bibstd/bible/reference.hpp>
#include <bibstd/bible/scripture_outline.hpp>
#include <bibstd/bible/scripture_types.hpp>
#include <bibstd/util/enum.hpp>
#include <bibstd/workflow/workflow_cache.hpp>
#include <bibstd/workflow/workflow_script.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>
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
  std::shared_ptr<workflow::workflow_cache> cache; // keeps the verses for the readers
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

  ///
  /// \return the passage of \p ref of \p file as its reader gives it, none if it has none
  ///
  [[nodiscard]] auto passage(const workflow::workflow_scripture_store::file_scripture& file, const bible::reference& ref) const
    -> std::optional<bible::passage>;
};

///
/// A scripture the bundled readers read, with the store whose readers keep its verses.
///
struct scripture_reading final
{
  std::shared_ptr<const scripture_store> fixture;
  std::optional<workflow::workflow_scripture_store::file_scripture> file; // none if no reader read it

  ///
  /// \return true if a reader read the scripture
  ///
  explicit operator bool() const;

  ///
  /// \return the outline of the scripture read
  ///
  auto operator->() const -> const bible::scripture_outline*;
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
  cache = std::make_shared<workflow::workflow_cache>(settings, script);
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
///
inline auto scripture_store::passage(
  const workflow::workflow_scripture_store::file_scripture& file, const bible::reference& ref
) const -> std::optional<bible::passage>
{
  const auto output = script->run<workflow::workflow_scripture::passage_manifest>(
    file.reader,
    {
      file.file_name,
      std::string{util::enum_name(ref.book())},
      static_cast<std::int64_t>(ref.chapter().value),
      static_cast<std::int64_t>(ref.verse().value),
    }
  );
  const auto markup = output ? output->get<"markup">() : std::nullopt;
  if(!markup)
  {
    return std::nullopt;
  }
  auto passage = bible::passage{ref, *markup, bible::passage_format::xml};
  passage.cross_references(output->get<"cross_references">().value_or(std::vector<std::string>{}));
  return passage;
}

///
///
inline scripture_reading::operator bool() const
{
  return file.has_value();
}

///
///
inline auto scripture_reading::operator->() const -> const bible::scripture_outline*
{
  return file->outline.get();
}

///
/// \return the scripture the bundled readers read from the scripture folder of \p fixture, empty if none
///
[[nodiscard]] inline auto reading_of(std::shared_ptr<const scripture_store> fixture) -> scripture_reading
{
  const auto scriptures = fixture->store->scriptures();
  auto file = scriptures.empty() ? std::nullopt : std::make_optional(scriptures.begin()->second);
  return scripture_reading{.fixture = std::move(fixture), .file = std::move(file)};
}

///
/// \return the scripture the bundled readers read from a zip archive of \p entries, empty if none
///
[[nodiscard]] inline auto read_scripture(const std::string_view name, const std::vector<io::test_utils::zip_entry>& entries)
  -> scripture_reading
{
  return reading_of(std::make_shared<const scripture_store>(name, std::map<std::string, std::vector<io::test_utils::zip_entry>>{{"scripture.zip", entries}}));
}

///
/// \return the passage of \p ref in the scripture of \p reading, none if it has none
///
[[nodiscard]] inline auto passage_of(const scripture_reading& reading, const bible::reference& ref)
  -> std::optional<bible::passage>
{
  return reading.fixture->passage(*reading.file, ref);
}

///
/// \return the markup of the verse \p verse of \p chapter of \p book in the scripture of \p reading, none if it has none
///
[[nodiscard]] inline auto verse_of(
  const scripture_reading& reading, const bible::book_id book, const std::uint32_t chapter, const std::uint32_t verse
) -> std::optional<std::string>
{
  const auto passage = passage_of(reading, bible::reference::create_unguarded(book, chapter, verse));
  return passage ? std::optional{passage->xml()} : std::nullopt;
}

} // namespace bibstd::test_utils
