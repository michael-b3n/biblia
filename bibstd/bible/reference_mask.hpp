#pragma once

#include "bibstd/bible/reference_range.hpp"

#include <string>
#include <string_view>

namespace bibstd::bible
{

///
/// Mask turning a reference range into text, e.g. "[book] [#chapter_begin],[#verse_begin]{-[#verse_end]}".
/// An unknown key stays as typed. Text in { } drops out if all its variables are empty, a group without variables
/// stays as typed.
///
struct reference_mask final
{
  ///
  /// What the variables of a mask stand for. Unavailable ones are empty.
  ///
  struct values final
  {
    reference_range range;
    std::string content;
    std::string content_numbered;
    std::string book;
    std::string book_long;
    std::string book_abbreviation;
    std::string book_end;
    std::string scripture;
    std::string scripture_abbreviation;
    std::string copyright;
  };

  ///
  /// \return \p mask with its variables replaced
  ///
  [[nodiscard]] static auto render(std::string_view mask, const values& values) -> std::string;

  ///
  /// \return whether \p mask needs the text of the verses, which takes a script time to deliver
  ///
  [[nodiscard]] static auto uses_content(std::string_view mask) -> bool;
};

} // namespace bibstd::bible
