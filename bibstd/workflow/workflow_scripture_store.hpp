#pragma once

#include <arcstd/lua/script_table.hpp>
#include <arcstd/util/path.hpp>
#include <arcstd/workflow/workflow_base.hpp>
#include <arcstd/workflow/workflow_script.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace bibstd::bible
{
class scripture_outline;
} // namespace bibstd::bible

namespace bibstd::workflow
{

///
/// Workflow scripture store, holds the scriptures of the files in the scripture folder. Scripts read
/// the files: each script offering scripture.read is a reader, e.g. the bundled ones for USX and USFM.
/// A reader keeps the verses of a file, the store only what it tells about the file.
///
class workflow_scripture_store final : public arcstd::workflow::workflow_base<void>
{
  // Typedefs
  struct file_scripture_t final
  {
    std::shared_ptr<bible::scripture_outline> outline;
    arcstd::workflow::workflow_script::script_name_type reader; // gives the verses, \see workflow_scripture::passage_manifest
    std::string file_name;                    // the name the reader knows the scripture by
  };

  // Variables
  const std::shared_ptr<arcstd::workflow::workflow_script> workflow_script_;
  const std::filesystem::path folder_;
  mutable std::mutex mtx_;
  std::map<std::filesystem::path, file_scripture_t> files_; // the scriptures by file

public: // Typedefs
  using file_scripture = file_scripture_t;
  using scripture_map_type = std::map<std::string, file_scripture>;

  // Manifest of a script reading scripture files, \see doc/lua_api.md
  // clang-format off
  ///
  /// \lua A script that offers this reads scripture files, those of the scripture folder and those the user imports.
  /// The app asks each reader until one tells the file is of its format, by a name or an error. The app reads the
  /// archive, the script its entries with `workflow.scripture_store.read_entry`. The bundled readers `USX` and
  /// `USFM` read the USX bundles of the Digital Bible Library and zip archives of USFM files. A reader keeps the
  /// verses, e.g. in `workflow.cache`, and gives them by `scripture.passage` with the file name as `name`. The app
  /// asks on every start, a reader that kept the file at this size and time answers from what it kept.
  /// \param file the name of the file, e.g. `bible.zip`
  /// \param entries the names of the files in the archive
  /// \param size the size of the file in bytes
  /// \param modified the time the file was last changed, a number that grows with it
  /// \param reload true to read the file anew, e.g. on an import or a reload by the user
  /// \return name the name of the scripture, `nil` if the file is not of a format the script reads
  /// \return abbreviation its abbreviation
  /// \return language its language
  /// \return copyright its copyright, shown below the verses
  /// \return book_names the names of the books by the identifier of the app,
  /// e.g. `john`: `abbreviation`, `short_name` shown in the app, and `long_name`
  /// \return verse_counts the last verse of each chapter by book and chapter, the versification of the scripture.
  /// A book left out takes the one of the closest default versification.
  /// \return error why a file of its format can not be read
  ///
  struct read_manifest final
  {
    static inline const arcstd::util::path id{"scripture.read"};
    using input = arcstd::lua::script_table<
      arcstd::lua::field<"file", std::string>,
      arcstd::lua::field<"entries", std::vector<std::string>>,
      arcstd::lua::field<"size", std::int64_t>,
      arcstd::lua::field<"modified", std::int64_t>,
      arcstd::lua::field<"reload", bool>>;
    using output = arcstd::lua::script_table<
      arcstd::lua::field<"name", std::optional<std::string>>,
      arcstd::lua::field<"abbreviation", std::optional<std::string>>,
      arcstd::lua::field<"language", std::optional<std::string>>,
      arcstd::lua::field<"copyright", std::optional<std::string>>,
      arcstd::lua::field<"book_names", std::optional<std::map<std::string, std::map<std::string, std::string>>>>,
      arcstd::lua::field<"verse_counts", std::optional<std::map<std::string, std::map<std::int64_t, std::int64_t>>>>,
      arcstd::lua::field<"error", std::optional<std::string>>>;
  };
  // clang-format on

public: // Constants
  static constexpr auto folder_name = "scriptures";

public: // Structors
  workflow_scripture_store(
    std::shared_ptr<arcstd::workflow::workflow_settings> workflow_settings,
    std::shared_ptr<arcstd::workflow::workflow_script> workflow_script
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
  /// scriptures read before. Each is there once it is read, a file no reader reads is logged and left out.
  /// A reader answers from what it kept of a file it read before. Blocks until all are read.
  ///
  auto load() -> void;

  ///
  /// Like load, the readers read every file anew.
  ///
  auto reload() -> void;

  ///
  /// Take the file \p source, or every file directly inside the folder \p source, into the scripture
  /// folder, replacing a file of the same name. Each file is read before it is copied, so the folder only
  /// holds readable files: one that holds no scripture is logged and left where it is. Before the scripts
  /// are loaded there is no reader yet, then the files are copied and read once the scripts are loaded.
  /// \return number of files taken over
  ///
  [[nodiscard]] auto import(const std::filesystem::path& source) -> std::size_t;

private: // Implementation
  auto load(bool reload) -> void;
  [[nodiscard]] auto read(const std::filesystem::path& file, bool reload) const -> std::optional<file_scripture>;
};

} // namespace bibstd::workflow
