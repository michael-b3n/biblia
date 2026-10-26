#pragma once

#include "bibstd/lua/script_table.hpp"
#include "bibstd/util/path.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_script.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Forward declarations
namespace bibstd::bible
{
class scripture;
} // namespace bibstd::bible

namespace bibstd::workflow
{

///
/// Workflow scripture store, holds the scriptures of the files in the scripture folder. Scripts read
/// the files: each script offering scripture.read is a reader, e.g. the bundled ones for USX and USFM.
///
class workflow_scripture_store final : public workflow_base<void>
{
  // Variables
  const std::shared_ptr<workflow_script> workflow_script_;
  const std::filesystem::path folder_;
  mutable std::mutex mtx_;
  std::map<std::filesystem::path, std::shared_ptr<bible::scripture>> files_; // the scriptures by file

public: // Typedefs
  using scripture_map_type = std::map<std::string, std::shared_ptr<bible::scripture>>;

  // Manifest of a script reading scripture files, \see doc/lua_api.md
  // clang-format off
  ///
  /// \lua A script that offers this reads scripture files, those of the scripture folder and those the user imports.
  /// The app asks each reader until one tells the file is of its format, by a name or an error. The app reads the
  /// archive, the script its entries with `workflow.scripture_store.read_entry`. The bundled readers `USX` and
  /// `USFM` read the USX bundles of the Digital Bible Library and zip archives of USFM files.
  /// \param file the name of the file, e.g. `bible.zip`
  /// \param entries the names of the files in the archive
  /// \return name the name of the scripture, none if the file is not of a format the script reads
  /// \return abbreviation its abbreviation
  /// \return language its language
  /// \return copyright its copyright, shown below the verses
  /// \return book_names the names of the books by the identifier of the app,
  /// e.g. `john`: `abbreviation`, `short_name` shown in the app, and `long_name`
  /// \return verses the text of every verse the scripture numbers, by book, chapter and verse. The app derives the
  /// versification from them, a book left out takes the one of the closest default versification. Each verse is a
  /// sequence of paragraph sections `<p data-id="...">`: `begin` for a verse that opens its paragraph, `continue`
  /// for one that continues it and `undefined` outside of paragraphs. Inside, `<b>` and `<i>` mark text up, and
  /// `&`, `<` and `>` are escaped. A verse without text of its own is empty, e.g. 3 and 4 of a verse "2-4".
  /// \return cross_references the cross references of the verses by book, chapter and verse
  /// \return error why a file of its format can not be read
  ///
  struct read_manifest final
  {
    static inline const util::path id{"scripture.read"};
    using input = lua::script_table<lua::field<"file", std::string>, lua::field<"entries", std::vector<std::string>>>;
    using output = lua::script_table<
      lua::field<"name", std::optional<std::string>>,
      lua::field<"abbreviation", std::optional<std::string>>,
      lua::field<"language", std::optional<std::string>>,
      lua::field<"copyright", std::optional<std::string>>,
      lua::field<"book_names", std::optional<std::map<std::string, std::map<std::string, std::string>>>>,
      lua::field<"verses", std::optional<std::map<std::string, std::map<std::int64_t, std::map<std::int64_t, std::string>>>>>,
      lua::field<"cross_references", std::optional<std::map<std::string, std::map<std::int64_t, std::map<std::int64_t, std::vector<std::string>>>>>>,
      lua::field<"error", std::optional<std::string>>>;
  };
  // clang-format on

public: // Constants
  static constexpr auto folder_name = "scriptures";

public: // Structors
  workflow_scripture_store(
    std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script
  );
  ~workflow_scripture_store() noexcept override;

public: // Accessors
  ///
  /// \return the loaded scriptures by name, a name shared by several told apart by a counting suffix
  ///
  [[nodiscard]] auto scriptures() const -> scripture_map_type;

public: // Modifiers
  ///
  /// Read the files directly inside the scripture folder with the readers of the loaded scripts, replacing the
  /// scriptures read before. A file no reader reads is logged and left out. Blocks until all are read.
  ///
  auto load() -> void;

  ///
  /// Take the file \p source, or every file directly inside the folder \p source, into the scripture
  /// folder, replacing a file of the same name. Each file is read before it is copied, so the folder only
  /// holds readable files: one that holds no scripture is logged and left where it is. Before the scripts
  /// are loaded there is no reader yet, then the files are copied and read once the scripts are loaded.
  /// \return number of files taken over
  ///
  auto import(const std::filesystem::path& source) -> std::size_t;

private: // Implementation
  [[nodiscard]] auto read(const std::filesystem::path& file) const -> std::shared_ptr<bible::scripture>;
};

} // namespace bibstd::workflow
