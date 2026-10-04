#pragma once

#include "bibstd/util/language.hpp"

namespace bibstd::system
{

///
/// Locale of the user, as the operating system reports it.
///
struct locale final
{
  // Constants
  static constexpr auto fallback_language = util::language::english;

  // Static
  ///
  /// Get the first display language of the user that is a known language.
  /// \return preferred language, fallback_language if the user prefers no known language
  ///
  [[nodiscard]] static auto preferred_language() -> util::language;
};

} // namespace bibstd::system
