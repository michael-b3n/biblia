#pragma once

#include <cassert>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

namespace bibstd::txt
{

///
/// Find unsigned integer within string functor.
///
struct find_uint_t final
{
  ///
  /// Result of find_uint.
  ///
  struct result final
  {
    std::uint32_t value;           // Unsigned integer value found in string_view
    std::size_t post_value_offset; // Offset index after value from begin of provided text
    constexpr auto operator<=>(const result&) const = default;
  };

  ///
  /// Find unsigned integer within string view.
  /// \return result with value and meta information
  ///
  [[nodiscard]] constexpr auto operator()(std::string_view text) const -> std::optional<result>;
};

///
/// \see find_uint_t::operator()()
///
inline constexpr find_uint_t find_uint;

///
///
constexpr auto find_uint_t::operator()(const std::string_view text) const -> std::optional<result>
{
  std::uint32_t value{0};
  const auto retval = std::from_chars(text.data(), text.end(), value);
  if(retval.ec != decltype(retval.ec){})
  {
    return std::nullopt;
  }
  const auto offset = std::ranges::distance(text.begin(), retval.ptr);
  assert(offset >= 0);
  return result{.value = value, .post_value_offset = static_cast<std::size_t>(offset)};
}

} // namespace bibstd::txt
