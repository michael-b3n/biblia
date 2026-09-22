#pragma once

#include "bibstd/util/const_map.hpp"
#include "bibstd/util/enum.hpp"

namespace bibstd::util
{

///
/// Language enumeration.
///
enum class language
{
  english,
  german,
};

///
/// Language direction enumeration
///
enum class language_direction
{
  // Left to right: The most common global format, used by the Latin, Cyrillic, and Greek alphabets
  // (e.g., English, Spanish, Russian). Lines progress top-to-bottom.
  ltr,

  // Right to left: Used by scripts like Arabic, Hebrew, and Persian. Basic text flows rightward,
  // while numbers or embedded LTR words may flow leftward, making them bidirectional (bidi) text.
  rtl,

  // Vertical Top-to-bottom left to right scripts, such as Traditional Mongolian,
  // Todo Bichig, Manchu, Phags-pa.
  vertical_ltr,

  // Vertical Top-to-bottom right to left scripts, such as Traditional Chinese, Japanese, Korean,
  // Khitan Large ScriptJurchen Script.
  vertical_rtl
};

///
/// Language direction map, mapping a direction to each language.
///
inline constexpr auto language_direction_map = util::make_const_map<language, language_direction>({
  {language::english, language_direction::ltr},
  { language::german, language_direction::ltr},
});
static_assert(language_direction_map.size() == util::enum_count<language>());

} // namespace bibstd::util
