#pragma once

#include "bibstd/util/exception.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string_view>

namespace bibstd::util
{

///
/// String literal taken as compile time argument: template<util::string_literal Key>.
/// It takes only a literal ending with '\0'. Accessor `view()` holds all of its characters.
///
template<std::size_t N>
struct string_literal final
{
  // Variables
  char chars[N]{};

  // Structors
  ///
  /// String literal constructor. It is implicit, so a string literal converts to
  /// the template argument. An embedded or missing '\0' does not compile.
  ///
  consteval string_literal(const char (&s)[N]);

  // Static
  ///
  /// \return true if the last character of \p s is its only '\0'
  ///
  [[nodiscard]] static constexpr auto terminated_once(const char (&s)[N]) -> bool;

  // Accessors
  ///
  /// \return the characters without the terminating '\0'
  ///
  [[nodiscard]] constexpr auto view() const -> std::string_view;
};

///
///
template<std::size_t N>
consteval string_literal<N>::string_literal(const char (&s)[N])
{
  if(!terminated_once(s))
  {
    throw util::exception{"string literal with an embedded or missing '\\0'"};
  }
  std::ranges::copy(s, chars);
}

///
///
template<std::size_t N>
constexpr auto string_literal<N>::terminated_once(const char (&s)[N]) -> bool
{
  return std::ranges::find(s, '\0') == std::ranges::prev(std::ranges::end(s));
}

///
///
template<std::size_t N>
constexpr auto string_literal<N>::view() const -> std::string_view
{
  return {chars, N - 1};
}

} // namespace bibstd::util
