#pragma once

#include "bibstd/bible/common.hpp"
#include "bibstd/bible/scripture.hpp"
#include "bibstd/bible/scripture_reader.hpp"
#include "bibstd/bible/scripture_types.hpp"
#include "bibstd/util/const_map.hpp"
#include "bibstd/util/enum.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

// Forward declarations
namespace bibstd::io
{
class zip_file_reader;
} // namespace bibstd::io

namespace bibstd::bible
{

///
/// Reads the USX bundle format: a zip archive carrying a "metadata.xml" descriptor in its root and
/// the USX document of every bible book, each named after the book's USX abbreviation.
///
class scripture_reader_usx final : public scripture_reader
{
  // Constants
  static constexpr auto books = util::make_const_bimap<book_id, std::string_view>({
    {        book_id::genesis, "GEN"},
    {         book_id::exodus, "EXO"},
    {      book_id::leviticus, "LEV"},
    {        book_id::numbers, "NUM"},
    {    book_id::deuteronomy, "DEU"},
    {         book_id::joshua, "JOS"},
    {         book_id::judges, "JDG"},
    {           book_id::ruth, "RUT"},
    {        book_id::samuel1, "1SA"},
    {        book_id::samuel2, "2SA"},
    {         book_id::kings1, "1KI"},
    {         book_id::kings2, "2KI"},
    {    book_id::chronicles1, "1CH"},
    {    book_id::chronicles2, "2CH"},
    {           book_id::ezra, "EZR"},
    {       book_id::nehemiah, "NEH"},
    {         book_id::esther, "EST"},
    {            book_id::job, "JOB"},
    {         book_id::psalms, "PSA"},
    {       book_id::proverbs, "PRO"},
    {   book_id::ecclesiastes, "ECC"},
    {book_id::song_of_solomon, "SNG"},
    {         book_id::isaiah, "ISA"},
    {       book_id::jeremiah, "JER"},
    {   book_id::lamentations, "LAM"},
    {        book_id::ezekiel, "EZK"},
    {         book_id::daniel, "DAN"},
    {          book_id::hosea, "HOS"},
    {           book_id::joel, "JOL"},
    {           book_id::amos, "AMO"},
    {        book_id::obadiah, "OBA"},
    {          book_id::jonah, "JON"},
    {          book_id::micah, "MIC"},
    {          book_id::nahum, "NAM"},
    {       book_id::habakkuk, "HAB"},
    {      book_id::zephaniah, "ZEP"},
    {         book_id::haggai, "HAG"},
    {      book_id::zechariah, "ZEC"},
    {        book_id::malachi, "MAL"},
    {        book_id::matthew, "MAT"},
    {           book_id::mark, "MRK"},
    {           book_id::luke, "LUK"},
    {           book_id::john, "JHN"},
    {           book_id::acts, "ACT"},
    {         book_id::romans, "ROM"},
    {   book_id::corinthians1, "1CO"},
    {   book_id::corinthians2, "2CO"},
    {      book_id::galatians, "GAL"},
    {      book_id::ephesians, "EPH"},
    {    book_id::philippians, "PHP"},
    {     book_id::colossians, "COL"},
    { book_id::thessalonians1, "1TH"},
    { book_id::thessalonians2, "2TH"},
    {       book_id::timothy1, "1TI"},
    {       book_id::timothy2, "2TI"},
    {          book_id::titus, "TIT"},
    {       book_id::philemon, "PHM"},
    {        book_id::hebrews, "HEB"},
    {          book_id::james, "JAS"},
    {         book_id::peter1, "1PE"},
    {         book_id::peter2, "2PE"},
    {          book_id::john1, "1JN"},
    {          book_id::john2, "2JN"},
    {          book_id::john3, "3JN"},
    {           book_id::jude, "JUD"},
    {     book_id::revelation, "REV"}
  });
  static_assert(books.size() == util::enum_count<book_id>());

  // Typedefs
  ///
  /// Name and passages parsed out of the USX document of a single book.
  ///
  struct book_document final
  {
    book_name name;
    scripture::passage_map_type passages;
  };

  ///
  /// Names and passages of every book of a scripture.
  ///
  struct scripture_content final
  {
    scripture::book_name_map_type book_names;
    scripture::passage_map_type passages;
  };

public: // Overrides
  ///
  /// \see scripture_reader::name
  ///
  auto name() const -> name_type override;

  ///
  /// A USX bundle is identified by the "metadata.xml" descriptor in its archive root.
  /// \see scripture_reader::recognizes
  ///
  auto recognizes(const io::zip_file_reader& archive) const -> bool override;

  ///
  /// \see scripture_reader::read
  ///
  auto read(const io::zip_file_reader& archive) const -> std::unique_ptr<scripture> override;

private: // Implementation
  ///
  /// Parse the "metadata.xml" descriptor of the bundle.
  /// \return Information about the scripture, or std::nullopt if it is missing or broken
  ///
  [[nodiscard]] static auto parse_metadata(const io::zip_file_reader& archive) -> std::optional<scripture_info>;

  ///
  /// Parse the USX document of a single book.
  /// \return Name and passages of the book, or std::nullopt if the document cannot be parsed
  ///
  [[nodiscard]] static auto parse_book_document(book_id book, const std::string& usx_content) -> std::optional<book_document>;

  ///
  /// Load and parse the USX document of every book of the scripture.
  /// \return Names and passages of all books, or std::nullopt if one is missing or broken
  ///
  [[nodiscard]] static auto load_books(const io::zip_file_reader& archive) -> std::optional<scripture_content>;
};

} // namespace bibstd::bible
