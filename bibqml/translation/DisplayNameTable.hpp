#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace bibqml
{

///
/// Table of the display names of an application in all languages, read from a CSV document:
/// \code
///   key,<language>,<language>,...
///   <key>,<display name>,<display name>,...
/// \endcode
/// A key is an identifier of the backend, e.g. a segment of a setting path. A language column is named by the
/// identifier used for lookups. Lines starting with '#' and empty lines are ignored.
///
class DisplayNameTable final
{
  // Variables
  std::vector<std::string> languages_;
  std::unordered_map<std::string, std::vector<std::string>> entries_;

public: // Structors
  ///
  /// Construct the table from a CSV document, an empty one holds no names.
  /// A malformed row is dropped, a malformed document rejected: one bad row shall not cost
  /// every other name.
  /// \throws util::exception if the document does not describe a table of display names
  ///
  explicit DisplayNameTable(std::span<const std::byte> csv);

public: // Accessors
  ///
  /// Access all available languages in the order defined by the document.
  /// \return list of language identifiers
  ///
  [[nodiscard]] auto languages() const -> const std::vector<std::string>&;

  ///
  /// Access the display name of a key.
  /// \return display name, std::nullopt if the language or the key is unknown
  ///
  [[nodiscard]] auto name(std::string_view language, std::string_view key) const -> std::optional<std::string>;
};

} // namespace bibqml
