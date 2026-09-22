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
};

///
/// A single bible verse together with its cross references, its content marked up as passage_markup.
///
struct passage final
{
  // Variables
  reference ref;
  std::string content;
  std::vector<std::string> cross_references;

  // Operators
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
