#pragma once

#include "bibstd/util/const_map.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/language.hpp"

#include <optional>
#include <string_view>

namespace bibstd::system
{

///
/// Language tag map, mapping the primary BCP-47 subtag windows names a language by to each language.
///
inline constexpr auto language_wtag_map = util::make_const_bimap<util::language, std::wstring_view>({
  {util::language::english, L"en"},
  { util::language::german, L"de"},
});
static_assert(language_wtag_map.size() == util::enum_count<util::language>());

///
/// Get the language of a BCP-47 tag, e.g. "de-CH". Only the primary subtag is compared.
/// \return language, std::nullopt if the primary subtag names no known language
///
constexpr auto from_language_tag(const std::wstring_view tag) -> std::optional<util::language>
{
  const auto primary_subtag = tag.substr(0, tag.find_first_of(L"-_"));
  if(language_wtag_map.contains(primary_subtag))
  {
    return language_wtag_map.at(primary_subtag);
  }
  return std::nullopt;
}

} // namespace bibstd::system
