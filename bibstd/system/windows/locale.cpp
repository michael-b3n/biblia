#include "bibstd/system/locale.hpp"
#include "bibstd/system/windows/language_tag.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/log.hpp"

#include "bibstd/system/windows/win.hpp"

#include <ranges>
#include <string>
#include <string_view>

namespace bibstd::system
{

///
///
auto locale::preferred_language() -> util::language
{
  auto count = ULONG{0};
  auto size = ULONG{0};
  if(!static_cast<bool>(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, nullptr, &size)))
  {
    LOG_WARN("preferred languages unavailable: error={}", GetLastError());
    return fallback_language;
  }
  auto buffer = std::wstring(size, L'\0');
  if(!static_cast<bool>(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, buffer.data(), &size)))
  {
    LOG_WARN("preferred languages unavailable: error={}", GetLastError());
    return fallback_language;
  }
  // The buffer holds null separated tags in order of preference, the first known one wins.
  for(const auto tag : std::wstring_view{buffer} | std::views::split(L'\0'))
  {
    if(const auto language = from_language_tag(std::wstring_view{tag}))
    {
      LOG_INFO("preferred language found: \"{}\"", util::enum_name(*language));
      return *language;
    }
  }
  LOG_INFO("no known preferred language: using fallback \"{}\"", util::enum_name(fallback_language));
  return fallback_language;
}

} // namespace bibstd::system
