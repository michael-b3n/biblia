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
/// Table holding the display names of an application in all available languages.
/// The table is read from a CSV document with the following layout:
/// \code
///   key,<language>,<language>,...
///   <key>,<display name>,<display name>,...
/// \endcode
/// The first column is named `key` and holds the keys, all following columns hold the display
/// names of one language each. The name of a language column is the language identifier used
/// for lookups. Lines starting with '#' and empty lines are ignored.
///
/// Keys are the identifiers used by the backend, e.g. the path of a setting. The display name
/// of a setting value is stored under the key "<setting path>/<setting value>".
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
