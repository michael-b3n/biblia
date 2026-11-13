#pragma once

#include "bibocr/util/language.hpp"

#include <arcstd/util/const_map.hpp>
#include <arcstd/util/enum.hpp>

#include <optional>
#include <string_view>

namespace bibocr::system
{

///
/// Language tag map, mapping the primary BCP-47 subtag windows names a language by to each language.
///
inline constexpr auto language_wtag_map = arcstd::util::make_const_bimap<bibocr::util::language, std::wstring_view>({
  {bibocr::util::language::english, L"en"},
  { bibocr::util::language::german, L"de"},
});
static_assert(language_wtag_map.size() == arcstd::util::enum_count<bibocr::util::language>());

///
/// Get the language of a BCP-47 tag, e.g. "de-CH". Only the primary subtag is compared.
/// \return language, std::nullopt if the primary subtag names no known language
///
[[nodiscard]] constexpr auto from_language_tag(const std::wstring_view tag) -> std::optional<bibocr::util::language>
{
  const auto primary_subtag = tag.substr(0, tag.find_first_of(L"-_"));
  if(language_wtag_map.contains(primary_subtag))
  {
    return language_wtag_map.at(primary_subtag);
  }
  return std::nullopt;
}

} // namespace bibocr::system
