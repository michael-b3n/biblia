#pragma once

#include "arca/util/enum.hpp"
#include "arca/util/string.hpp"
#include "bibstd/bible/common.hpp"

#include <tuple>
#include <utility>

namespace bibstd::bible
{

///
/// Holds all german bible book name variants corresponding to book id.
///
struct ocr_book_variants_de final
{
  // Constants
  ///
  /// All german bible book name variants listed with corresponding book id.
  /// Whitespaces and fullstops are omitted.
  ///
  // clang-format off
  static constexpr auto name_variants = std::tuple{
    std::pair{        book_id::genesis,       util::string::to_string_view_array("Genesis", "1Mose", "Gen", "1Mo")},
    std::pair{         book_id::exodus,         util::string::to_string_view_array("Exodus", "2Mose", "Ex", "2Mo")},
    std::pair{      book_id::leviticus,     util::string::to_string_view_array("Levitikus", "3Mose", "Lev", "3Mo")},
    std::pair{        book_id::numbers,        util::string::to_string_view_array("Numeri", "4Mose", "Num", "4Mo")},
    std::pair{    book_id::deuteronomy, util::string::to_string_view_array("Deuteronomium", "5Mose", "Dtn", "5Mo")},
    std::pair{         book_id::joshua,                         util::string::to_string_view_array("Josua", "Jos")},
    std::pair{         book_id::judges,                        util::string::to_string_view_array("Richter", "Ri")},
    std::pair{           book_id::ruth,                                  util::string::to_string_view_array("Rut")},
    std::pair{        book_id::samuel1,                      util::string::to_string_view_array("1Samuel", "1Sam")},
    std::pair{        book_id::samuel2,                      util::string::to_string_view_array("2Samuel", "2Sam")},
    std::pair{         book_id::kings1,               util::string::to_string_view_array("1Könige", "1Kön", "1Kö")},
    std::pair{         book_id::kings2,               util::string::to_string_view_array("2Könige", "2Kön", "2Kö")},
    std::pair{    book_id::chronicles1,                     util::string::to_string_view_array("1Chronik", "1Chr")},
    std::pair{    book_id::chronicles2,                     util::string::to_string_view_array("2Chronik", "2Chr")},
    std::pair{           book_id::ezra,                          util::string::to_string_view_array("Esra", "Esr")},
    std::pair{       book_id::nehemiah,                       util::string::to_string_view_array("Nehemia", "Neh")},
    std::pair{         book_id::esther,                         util::string::to_string_view_array("Ester", "Est")},
    std::pair{            book_id::job,                           util::string::to_string_view_array("Hiob", "Hi")},
    std::pair{         book_id::psalms,                          util::string::to_string_view_array("Psalm", "Ps")},
    std::pair{       book_id::proverbs,                       util::string::to_string_view_array("Sprüche", "Spr")},
    std::pair{   book_id::ecclesiastes,                     util::string::to_string_view_array("Prediger", "Pred")},
    std::pair{book_id::song_of_solomon,              util::string::to_string_view_array("Hohelied", "Hhld", "Hld")},
    std::pair{         book_id::isaiah,                        util::string::to_string_view_array("Jesaja", "Jes")},
    std::pair{       book_id::jeremiah,                       util::string::to_string_view_array("Jeremia", "Jer")},
    std::pair{   book_id::lamentations,                  util::string::to_string_view_array("Klagelieder", "Klgl")},
    std::pair{        book_id::ezekiel,                      util::string::to_string_view_array("Hesekiel", "Hes")},
    std::pair{         book_id::daniel,                        util::string::to_string_view_array("Daniel", "Dan")},
    std::pair{          book_id::hosea,                         util::string::to_string_view_array("Hosea", "Hos")},
    std::pair{           book_id::joel,                                 util::string::to_string_view_array("Joel")},
    std::pair{           book_id::amos,                           util::string::to_string_view_array("Amos", "Am")},
    std::pair{        book_id::obadiah,                        util::string::to_string_view_array("Obadja", "Obd")},
    std::pair{          book_id::jonah,                                 util::string::to_string_view_array("Jona")},
    std::pair{          book_id::micah,                          util::string::to_string_view_array("Micha", "Mi")},
    std::pair{          book_id::nahum,                         util::string::to_string_view_array("Nahum", "Nah")},
    std::pair{       book_id::habakkuk,                       util::string::to_string_view_array("Habakuk", "Hab")},
    std::pair{      book_id::zephaniah,                       util::string::to_string_view_array("Zefanja", "Zef")},
    std::pair{         book_id::haggai,                        util::string::to_string_view_array("Haggai", "Hag")},
    std::pair{      book_id::zechariah,                     util::string::to_string_view_array("Sacharja", "Sach")},
    std::pair{        book_id::malachi,                      util::string::to_string_view_array("Maleachi", "Mal")},
    std::pair{        book_id::matthew,                       util::string::to_string_view_array("Matthäus", "Mt")},
    std::pair{           book_id::mark,                         util::string::to_string_view_array("Markus", "Mk")},
    std::pair{           book_id::luke,                          util::string::to_string_view_array("Lukas", "Lk")},
    std::pair{           book_id::john,                      util::string::to_string_view_array("Johannes", "Joh")},
    std::pair{           book_id::acts,             util::string::to_string_view_array("Apostelgeschichte", "Apg")},
    std::pair{         book_id::romans,                         util::string::to_string_view_array("Römer", "Röm")},
    std::pair{   book_id::corinthians1,                   util::string::to_string_view_array("1Korinther", "1Kor")},
    std::pair{   book_id::corinthians2,                   util::string::to_string_view_array("2Korinther", "2Kor")},
    std::pair{      book_id::galatians,                       util::string::to_string_view_array("Galater", "Gal")},
    std::pair{      book_id::ephesians,                       util::string::to_string_view_array("Epheser", "Eph")},
    std::pair{    book_id::philippians,                    util::string::to_string_view_array("Philipper", "Phil")},
    std::pair{     book_id::colossians,                      util::string::to_string_view_array("Kolosser", "Kol")},
    std::pair{ book_id::thessalonians1,            util::string::to_string_view_array("1Thessalonicher", "1Thess")},
    std::pair{ book_id::thessalonians2,            util::string::to_string_view_array("2Thessalonicher", "2Thess")},
    std::pair{       book_id::timothy1,                   util::string::to_string_view_array("1Timotheus", "1Tim")},
    std::pair{       book_id::timothy2,                   util::string::to_string_view_array("2Timotheus", "2Tim")},
    std::pair{          book_id::titus,                         util::string::to_string_view_array("Titus", "Tit")},
    std::pair{       book_id::philemon,                     util::string::to_string_view_array("Philemon", "Phlm")},
    std::pair{        book_id::hebrews,                      util::string::to_string_view_array("Hebräer", "Hebr")},
    std::pair{          book_id::james,                       util::string::to_string_view_array("Jakobus", "Jak")},
    std::pair{         book_id::peter1,                     util::string::to_string_view_array("1Petrus", "1Petr")},
    std::pair{         book_id::peter2,                     util::string::to_string_view_array("2Petrus", "2Petr")},
    std::pair{          book_id::john1,                    util::string::to_string_view_array("1Johannes", "1Joh")},
    std::pair{          book_id::john2,                    util::string::to_string_view_array("2Johannes", "2Joh")},
    std::pair{          book_id::john3,                    util::string::to_string_view_array("3Johannes", "3Joh")},
    std::pair{           book_id::jude,                         util::string::to_string_view_array("Judas", "Jud")},
    std::pair{     book_id::revelation,        util::string::to_string_view_array("Offenbarung", "Offenb", "Offb")}
  };
  // clang-format on
  static_assert(std::tuple_size_v<decltype(name_variants)> == util::enum_count<book_id>());
};

} // namespace bibstd::bible
