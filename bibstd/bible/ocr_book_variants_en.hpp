#pragma once

#include "arcstd/util/enum.hpp"
#include "arcstd/util/string.hpp"
#include "bibstd/bible/common.hpp"

#include <tuple>
#include <utility>

namespace bibstd::bible
{

///
/// Holds all english bible book name variants corresponding to book id.
///
struct ocr_book_variants_en final
{
  // Constants
  ///
  /// All english bible book name variants listed with corresponding book id.
  /// Whitespaces and fullstops are omitted.
  ///
  // clang-format off
  static constexpr auto name_variants = std::tuple{
    std::pair{        book_id::genesis,               arcstd::util::string::to_string_view_array("Genesis", "Gen")},
    std::pair{         book_id::exodus,         arcstd::util::string::to_string_view_array("Exodus", "Exod", "Ex")},
    std::pair{      book_id::leviticus,             arcstd::util::string::to_string_view_array("Leviticus", "Lev")},
    std::pair{        book_id::numbers,               arcstd::util::string::to_string_view_array("Numbers", "Num")},
    std::pair{    book_id::deuteronomy,    arcstd::util::string::to_string_view_array("Deuteronomy", "Deut", "Dt")},
    std::pair{         book_id::joshua,               arcstd::util::string::to_string_view_array("Joshua", "Josh")},
    std::pair{         book_id::judges,               arcstd::util::string::to_string_view_array("Judges", "Judg")},
    std::pair{           book_id::ruth,                         arcstd::util::string::to_string_view_array("Ruth")},
    std::pair{        book_id::samuel1,              arcstd::util::string::to_string_view_array("1Samuel", "1Sam")},
    std::pair{        book_id::samuel2,              arcstd::util::string::to_string_view_array("2Samuel", "2Sam")},
    std::pair{         book_id::kings1,               arcstd::util::string::to_string_view_array("1Kings", "1Kgs")},
    std::pair{         book_id::kings2,               arcstd::util::string::to_string_view_array("2Kings", "2Kgs")},
    std::pair{    book_id::chronicles1,          arcstd::util::string::to_string_view_array("1Chronicles", "1Chr")},
    std::pair{    book_id::chronicles2,          arcstd::util::string::to_string_view_array("2Chronicles", "2Chr")},
    std::pair{           book_id::ezra,                  arcstd::util::string::to_string_view_array("Ezra", "Ezr")},
    std::pair{       book_id::nehemiah,              arcstd::util::string::to_string_view_array("Nehemiah", "Neh")},
    std::pair{         book_id::esther,               arcstd::util::string::to_string_view_array("Esther", "Esth")},
    std::pair{            book_id::job,                          arcstd::util::string::to_string_view_array("Job")},
    std::pair{         book_id::psalms,        arcstd::util::string::to_string_view_array("Psalms", "Ps", "Psalm")},
    std::pair{       book_id::proverbs,             arcstd::util::string::to_string_view_array("Proverbs", "Prov")},
    std::pair{   book_id::ecclesiastes,         arcstd::util::string::to_string_view_array("Ecclesiastes", "Eccl")},
    std::pair{book_id::song_of_solomon, arcstd::util::string::to_string_view_array("SongofSolomon", "Song", "SoS")},
    std::pair{         book_id::isaiah,                arcstd::util::string::to_string_view_array("Isaiah", "Isa")},
    std::pair{       book_id::jeremiah,              arcstd::util::string::to_string_view_array("Jeremiah", "Jer")},
    std::pair{   book_id::lamentations,          arcstd::util::string::to_string_view_array("Lamentations", "Lam")},
    std::pair{        book_id::ezekiel,              arcstd::util::string::to_string_view_array("Ezekiel", "Ezek")},
    std::pair{         book_id::daniel,                arcstd::util::string::to_string_view_array("Daniel", "Dan")},
    std::pair{          book_id::hosea,                 arcstd::util::string::to_string_view_array("Hosea", "Hos")},
    std::pair{           book_id::joel,                         arcstd::util::string::to_string_view_array("Joel")},
    std::pair{           book_id::amos,                   arcstd::util::string::to_string_view_array("Amos", "Am")},
    std::pair{        book_id::obadiah,              arcstd::util::string::to_string_view_array("Obadiah", "Obad")},
    std::pair{          book_id::jonah,                 arcstd::util::string::to_string_view_array("Jonah", "Jon")},
    std::pair{          book_id::micah,                 arcstd::util::string::to_string_view_array("Micah", "Mic")},
    std::pair{          book_id::nahum,                 arcstd::util::string::to_string_view_array("Nahum", "Nah")},
    std::pair{       book_id::habakkuk,              arcstd::util::string::to_string_view_array("Habakkuk", "Hab")},
    std::pair{      book_id::zephaniah,            arcstd::util::string::to_string_view_array("Zephaniah", "Zeph")},
    std::pair{         book_id::haggai,                arcstd::util::string::to_string_view_array("Haggai", "Hag")},
    std::pair{      book_id::zechariah,            arcstd::util::string::to_string_view_array("Zechariah", "Zech")},
    std::pair{        book_id::malachi,               arcstd::util::string::to_string_view_array("Malachi", "Mal")},
    std::pair{        book_id::matthew,        arcstd::util::string::to_string_view_array("Matthew", "Matt", "Mt")},
    std::pair{           book_id::mark,                   arcstd::util::string::to_string_view_array("Mark", "Mk")},
    std::pair{           book_id::luke,                   arcstd::util::string::to_string_view_array("Luke", "Lk")},
    std::pair{           book_id::john,                   arcstd::util::string::to_string_view_array("John", "Jn")},
    std::pair{           book_id::acts,                         arcstd::util::string::to_string_view_array("Acts")},
    std::pair{         book_id::romans,                arcstd::util::string::to_string_view_array("Romans", "Rom")},
    std::pair{   book_id::corinthians1,         arcstd::util::string::to_string_view_array("1Corinthians", "1Cor")},
    std::pair{   book_id::corinthians2,         arcstd::util::string::to_string_view_array("2Corinthians", "2Cor")},
    std::pair{      book_id::galatians,             arcstd::util::string::to_string_view_array("Galatians", "Gal")},
    std::pair{      book_id::ephesians,             arcstd::util::string::to_string_view_array("Ephesians", "Eph")},
    std::pair{    book_id::philippians,          arcstd::util::string::to_string_view_array("Philippians", "Phil")},
    std::pair{     book_id::colossians,            arcstd::util::string::to_string_view_array("Colossians", "Col")},
    std::pair{ book_id::thessalonians1,     arcstd::util::string::to_string_view_array("1Thessalonians", "1Thess")},
    std::pair{ book_id::thessalonians2,     arcstd::util::string::to_string_view_array("2Thessalonians", "2Thess")},
    std::pair{       book_id::timothy1,             arcstd::util::string::to_string_view_array("1Timothy", "1Tim")},
    std::pair{       book_id::timothy2,             arcstd::util::string::to_string_view_array("2Timothy", "2Tim")},
    std::pair{          book_id::titus,                 arcstd::util::string::to_string_view_array("Titus", "Tit")},
    std::pair{       book_id::philemon,             arcstd::util::string::to_string_view_array("Philemon", "Phlm")},
    std::pair{        book_id::hebrews,               arcstd::util::string::to_string_view_array("Hebrews", "Heb")},
    std::pair{          book_id::james,                 arcstd::util::string::to_string_view_array("James", "Jas")},
    std::pair{         book_id::peter1,               arcstd::util::string::to_string_view_array("1Peter", "1Pet")},
    std::pair{         book_id::peter2,               arcstd::util::string::to_string_view_array("2Peter", "2Pet")},
    std::pair{          book_id::john1,                 arcstd::util::string::to_string_view_array("1John", "1Jn")},
    std::pair{          book_id::john2,                 arcstd::util::string::to_string_view_array("2John", "2Jn")},
    std::pair{          book_id::john3,                 arcstd::util::string::to_string_view_array("3John", "3Jn")},
    std::pair{           book_id::jude,                  arcstd::util::string::to_string_view_array("Jude", "Jud")},
    std::pair{     book_id::revelation,            arcstd::util::string::to_string_view_array("Revelation", "Rev")}
  };
  // clang-format on
  static_assert(std::tuple_size_v<decltype(name_variants)> == arcstd::util::enum_count<book_id>());
};

} // namespace bibstd::bible
