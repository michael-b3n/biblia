#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
namespace bibstd::bible
{
class scripture;
class scripture_reader;
} // namespace bibstd::bible

namespace bibstd::core
{

///
/// Core scripture store. This class contains the scripture data loaded from the files of a folder.
///
class core_scripture_store final
{
  // Variables
  const std::filesystem::path folder_;
  const std::vector<std::unique_ptr<const bible::scripture_reader>> readers_;
  std::map<std::filesystem::path, std::shared_ptr<bible::scripture>> scripture_files_;
  std::map<std::string, std::shared_ptr<bible::scripture>> scripture_data_;

public: // Typedefs
  using scripture_map_type = decltype(scripture_data_);

  ///
  /// Container types a scripture file ships as, told apart by the file extension. Which format the
  /// container holds is a separate question, answered by the readers, see bible::scripture_reader.
  ///
  enum class container_type
  {
    zip,
  };

public: // Structors
  ///
  /// Load every supported file directly inside \p folder, in the order of the file names.
  /// A file that fails to load is logged and left out.
  ///
  explicit core_scripture_store(std::filesystem::path folder);
  ~core_scripture_store() noexcept;

public: // Accessors
  ///
  /// \return Map of all loaded scriptures
  ///
  [[nodiscard]] auto scriptures() const -> const scripture_map_type&;

public: // Modifiers
  ///
  /// Take every supported file directly inside \p source into the folder of this store, replacing
  /// a file of the same name. Every file is read once, before it is copied: one that holds no
  /// scripture data is logged and left where it is, so the folder only holds loadable files.
  /// \return number of files taken over
  ///
  auto import(const std::filesystem::path& source) -> std::size_t;

private: // Implementation
  auto load() -> void;
  auto name_scriptures() -> void;
  [[nodiscard]] auto read(const std::filesystem::path& file) const -> std::shared_ptr<bible::scripture>;
};

} // namespace bibstd::core
