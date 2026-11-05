#pragma once

#include "bibstd/bible/reference.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::bible
{

///
/// The XML a scripture reader puts into the content of a passage. It carries the structure of the
/// verse, not its presentation, and text content is escaped so that a passage parses as XML.
///
struct passage_markup final
{
  // Constants
  ///
  /// Nodes. A verse is a sequence of paragraph sections, the inline nodes mark up their text.
  ///
  static constexpr std::string_view paragraph = "p";
  static constexpr std::string_view bold = "b";
  static constexpr std::string_view italic = "i";
  static constexpr std::string_view name_of_god = italic;
  static constexpr std::string_view translator_addition = italic;

  ///
  /// Attribute of a paragraph section, telling where the verse sits in the paragraph it belongs to:
  /// it either opens the paragraph or continues one that an earlier verse opened. A verse spanning
  /// several paragraphs has one section per paragraph.
  ///
  static constexpr std::string_view paragraph_attribute = "data-id";
  static constexpr std::string_view paragraph_begin = "begin";
  static constexpr std::string_view paragraph_continue = "continue";
  static constexpr std::string_view paragraph_undefined = "undefined"; // outside of any paragraph

  // Functions
  ///
  /// \return \p text as XML text content, the characters that carry meaning in XML replaced by their entity
  ///
  [[nodiscard]] static auto escaped_xml(std::string_view text) -> std::string;

  ///
  /// \return paragraph section of \p content, which is markup already, at \p position in its paragraph
  ///
  [[nodiscard]] static auto section(std::string_view position, std::string_view content) -> std::string;

  ///
  /// \return the plain text \p plain as XML, a section outside of any paragraph
  ///
  [[nodiscard]] static auto to_xml(std::string_view plain) -> std::string;

  ///
  /// \return the XML \p xml as plain text, its sections separated by a space and whitespace
  /// collapsed, \p xml itself if it is no XML of a passage. In this case a warning is logged.
  ///
  [[nodiscard]] static auto to_plain(std::string_view xml) -> std::string;
};

///
/// Format the content of a passage is provided in.
///
enum class passage_format
{
  xml,
  plain,
};

///
/// A single bible verse with its cross references. Its content stays as provided, \see passage_markup.
///
class passage final
{
  // Variables
  reference ref_;
  std::string content_;
  passage_format format_;
  std::vector<std::string> cross_references_;

public: // Structors
  ///
  /// \p content is provided in \p format.
  ///
  passage(reference ref, std::string content, passage_format format);

public: // Accessors
  ///
  /// \return the reference of the verse
  ///
  [[nodiscard]] auto ref() const -> const reference&;

  ///
  /// \return the cross references of the verse
  ///
  [[nodiscard]] auto cross_references() const -> const std::vector<std::string>&;

  ///
  /// \return the content as XML
  ///
  [[nodiscard]] auto xml() const -> std::string;

  ///
  /// \return the content as plain text
  ///
  [[nodiscard]] auto plain() const -> std::string;

public: // Setters
  ///
  /// Set the cross references of the verse.
  ///
  auto cross_references(std::vector<std::string> cross_references) -> void;

public: // Operators
  auto operator==(const passage&) const -> bool = default;
};

///
/// Information about a scripture.
///
struct scripture_info final
{
  // Variables
  std::string name;
  std::string abbreviation;
  std::string language;
  std::optional<std::string> copyright;

  // Operators
  auto operator==(const scripture_info&) const -> bool = default;
};

///
/// Names of a single book in the language of the scripture. Not every scripture provides all of the
/// forms, unavailable ones are empty.
///
struct book_name final
{
  // Variables
  std::string abbreviation; // abbreviated form
  std::string short_name;   // form intended for display
  std::string long_name;    // full title

  // Operators
  auto operator==(const book_name&) const -> bool = default;
};

} // namespace bibstd::bible
