#pragma once

#include "bibstd/bible/scripture.hpp"

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

// Forward declarations
namespace bibstd::bible
{
class scripture;
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
  std::map<std::filesystem::path, std::shared_ptr<bible::scripture>> scripture_files_;
  std::map<std::string, std::shared_ptr<bible::scripture>> scripture_data_;

public: // Typedefs
  using scripture_map_type = decltype(scripture_data_);

  ///
  /// Supported file types for scripture data.
  ///
  enum class supported_file_type
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
  [[nodiscard]] auto static read(const std::filesystem::path& file) -> std::shared_ptr<bible::scripture>;
};

} // namespace bibstd::core
