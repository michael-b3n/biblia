#pragma once

#include "arca/util/enum.hpp"
#include "arca/util/string.hpp"
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
    std::pair{        book_id::genesis,               util::string::to_string_view_array("Genesis", "Gen")},
    std::pair{         book_id::exodus,         util::string::to_string_view_array("Exodus", "Exod", "Ex")},
    std::pair{      book_id::leviticus,             util::string::to_string_view_array("Leviticus", "Lev")},
    std::pair{        book_id::numbers,               util::string::to_string_view_array("Numbers", "Num")},
    std::pair{    book_id::deuteronomy,    util::string::to_string_view_array("Deuteronomy", "Deut", "Dt")},
    std::pair{         book_id::joshua,               util::string::to_string_view_array("Joshua", "Josh")},
    std::pair{         book_id::judges,               util::string::to_string_view_array("Judges", "Judg")},
    std::pair{           book_id::ruth,                         util::string::to_string_view_array("Ruth")},
    std::pair{        book_id::samuel1,              util::string::to_string_view_array("1Samuel", "1Sam")},
    std::pair{        book_id::samuel2,              util::string::to_string_view_array("2Samuel", "2Sam")},
    std::pair{         book_id::kings1,               util::string::to_string_view_array("1Kings", "1Kgs")},
    std::pair{         book_id::kings2,               util::string::to_string_view_array("2Kings", "2Kgs")},
    std::pair{    book_id::chronicles1,          util::string::to_string_view_array("1Chronicles", "1Chr")},
    std::pair{    book_id::chronicles2,          util::string::to_string_view_array("2Chronicles", "2Chr")},
    std::pair{           book_id::ezra,                  util::string::to_string_view_array("Ezra", "Ezr")},
    std::pair{       book_id::nehemiah,              util::string::to_string_view_array("Nehemiah", "Neh")},
    std::pair{         book_id::esther,               util::string::to_string_view_array("Esther", "Esth")},
    std::pair{            book_id::job,                          util::string::to_string_view_array("Job")},
    std::pair{         book_id::psalms,        util::string::to_string_view_array("Psalms", "Ps", "Psalm")},
    std::pair{       book_id::proverbs,             util::string::to_string_view_array("Proverbs", "Prov")},
    std::pair{   book_id::ecclesiastes,         util::string::to_string_view_array("Ecclesiastes", "Eccl")},
    std::pair{book_id::song_of_solomon, util::string::to_string_view_array("SongofSolomon", "Song", "SoS")},
    std::pair{         book_id::isaiah,                util::string::to_string_view_array("Isaiah", "Isa")},
    std::pair{       book_id::jeremiah,              util::string::to_string_view_array("Jeremiah", "Jer")},
    std::pair{   book_id::lamentations,          util::string::to_string_view_array("Lamentations", "Lam")},
    std::pair{        book_id::ezekiel,              util::string::to_string_view_array("Ezekiel", "Ezek")},
    std::pair{         book_id::daniel,                util::string::to_string_view_array("Daniel", "Dan")},
    std::pair{          book_id::hosea,                 util::string::to_string_view_array("Hosea", "Hos")},
    std::pair{           book_id::joel,                         util::string::to_string_view_array("Joel")},
    std::pair{           book_id::amos,                   util::string::to_string_view_array("Amos", "Am")},
    std::pair{        book_id::obadiah,              util::string::to_string_view_array("Obadiah", "Obad")},
    std::pair{          book_id::jonah,                 util::string::to_string_view_array("Jonah", "Jon")},
    std::pair{          book_id::micah,                 util::string::to_string_view_array("Micah", "Mic")},
    std::pair{          book_id::nahum,                 util::string::to_string_view_array("Nahum", "Nah")},
    std::pair{       book_id::habakkuk,              util::string::to_string_view_array("Habakkuk", "Hab")},
    std::pair{      book_id::zephaniah,            util::string::to_string_view_array("Zephaniah", "Zeph")},
    std::pair{         book_id::haggai,                util::string::to_string_view_array("Haggai", "Hag")},
    std::pair{      book_id::zechariah,            util::string::to_string_view_array("Zechariah", "Zech")},
    std::pair{        book_id::malachi,               util::string::to_string_view_array("Malachi", "Mal")},
    std::pair{        book_id::matthew,        util::string::to_string_view_array("Matthew", "Matt", "Mt")},
    std::pair{           book_id::mark,                   util::string::to_string_view_array("Mark", "Mk")},
    std::pair{           book_id::luke,                   util::string::to_string_view_array("Luke", "Lk")},
    std::pair{           book_id::john,                   util::string::to_string_view_array("John", "Jn")},
    std::pair{           book_id::acts,                         util::string::to_string_view_array("Acts")},
    std::pair{         book_id::romans,                util::string::to_string_view_array("Romans", "Rom")},
    std::pair{   book_id::corinthians1,         util::string::to_string_view_array("1Corinthians", "1Cor")},
    std::pair{   book_id::corinthians2,         util::string::to_string_view_array("2Corinthians", "2Cor")},
    std::pair{      book_id::galatians,             util::string::to_string_view_array("Galatians", "Gal")},
    std::pair{      book_id::ephesians,             util::string::to_string_view_array("Ephesians", "Eph")},
    std::pair{    book_id::philippians,          util::string::to_string_view_array("Philippians", "Phil")},
    std::pair{     book_id::colossians,            util::string::to_string_view_array("Colossians", "Col")},
    std::pair{ book_id::thessalonians1,     util::string::to_string_view_array("1Thessalonians", "1Thess")},
    std::pair{ book_id::thessalonians2,     util::string::to_string_view_array("2Thessalonians", "2Thess")},
    std::pair{       book_id::timothy1,             util::string::to_string_view_array("1Timothy", "1Tim")},
    std::pair{       book_id::timothy2,             util::string::to_string_view_array("2Timothy", "2Tim")},
    std::pair{          book_id::titus,                 util::string::to_string_view_array("Titus", "Tit")},
    std::pair{       book_id::philemon,             util::string::to_string_view_array("Philemon", "Phlm")},
    std::pair{        book_id::hebrews,               util::string::to_string_view_array("Hebrews", "Heb")},
    std::pair{          book_id::james,                 util::string::to_string_view_array("James", "Jas")},
    std::pair{         book_id::peter1,               util::string::to_string_view_array("1Peter", "1Pet")},
    std::pair{         book_id::peter2,               util::string::to_string_view_array("2Peter", "2Pet")},
    std::pair{          book_id::john1,                 util::string::to_string_view_array("1John", "1Jn")},
    std::pair{          book_id::john2,                 util::string::to_string_view_array("2John", "2Jn")},
    std::pair{          book_id::john3,                 util::string::to_string_view_array("3John", "3Jn")},
    std::pair{           book_id::jude,                  util::string::to_string_view_array("Jude", "Jud")},
    std::pair{     book_id::revelation,            util::string::to_string_view_array("Revelation", "Rev")}
  };
  // clang-format on
  static_assert(std::tuple_size_v<decltype(name_variants)> == util::enum_count<book_id>());
};

} // namespace bibstd::bible
