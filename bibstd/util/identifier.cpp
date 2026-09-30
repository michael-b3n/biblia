#include "bibstd/util/identifier.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"

#include <ranges>
#include <string_view>

namespace bibstd::util
{
namespace
{

///
/// \return the normalized identifier of \p text, throws if it is empty
///
auto normalize(const std::string_view text) -> std::string
{
  if(text.empty())
  {
    throw util::exception{"identifier is empty"};
  }
  static constexpr auto to_valid_char = [](const char c) -> char
  {
    if(!identifier::characters.contains(c))
    {
      LOG_WARN("identifier character replaced: '{}' -> '_'", c);
      return '_';
    }
    return c;
  };
  return text | std::views::transform(to_valid_char) | std::ranges::to<std::string>();
}

} // namespace

///
///
identifier::identifier(const std::string_view text)
  : value_{normalize(text)}
{
}

///
///
auto identifier::from(const std::string_view text) -> std::optional<identifier>
{
  return text.empty() ? std::nullopt : std::optional{identifier{text}};
}

///
///
auto identifier::string() const -> const std::string&
{
  return value_;
}

} // namespace bibstd::util
