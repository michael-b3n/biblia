#pragma once

#include "bibstd/txt/find_uint.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/ranges.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace bibstd::util::string
{

///
/// Concept to check if T is string view type.
///
template<typename T>
concept string_view_type = std::is_convertible_v<T, std::string_view>;

///
/// Create string view array from string literal args.
/// \return string view array
///
template<string_view_type... S>
constexpr auto to_string_view_array(S&&... args) -> std::array<std::string_view, sizeof...(S)>
{
  return std::array{std::string_view{std::forward<decltype(args)>(args)}...};
}

///
/// Split string_view by delimiter and return vector of string parts.
/// \return vector of string parts
///
constexpr auto split(const std::string_view data, const auto delimiter) -> std::vector<std::string>
  requires(std::convertible_to<decltype(delimiter), std::string_view> || std::convertible_to<decltype(delimiter), char>)
{
  const auto delimiter_size = [&]() -> std::size_t
  {
    if constexpr(std::convertible_to<decltype(delimiter), std::string_view>)
    {
      return std::string_view(delimiter).size();
    }
    else
    {
      return 1;
    }
  }();
  auto result = std::vector<std::string>{};
  auto current_index = decltype(data.size()){0};

  std::ranges::for_each(
    util::ranges::index_view(data) |
      std::views::take_while([&]([[maybe_unused]] const auto /*i*/) { return current_index != std::string_view::npos; }),
    [&]([[maybe_unused]] const auto)
    {
      const auto pos = data.substr(current_index).find(delimiter);
      result.emplace_back(data.substr(current_index, pos));
      current_index = pos != std::string_view::npos ? current_index + pos + delimiter_size : std::string_view::npos;
    }
  );
  return result;
}

///
/// Join string parts with delimiter and return the joined string.
/// \return Joined string
///
constexpr auto join(const std::ranges::range auto& parts, const auto delimiter) -> std::string
  requires(
    std::convertible_to<std::ranges::range_value_t<decltype(parts)>, std::string_view> &&
    (std::convertible_to<decltype(delimiter), std::string_view> || std::convertible_to<decltype(delimiter), char>)
  )
{
  if(parts.empty())
  {
    return {};
  }
  return std::ranges::fold_left(
    std::ranges::next(std::ranges::cbegin(parts)),
    std::ranges::cend(parts),
    parts.front(),
    [&](const auto& a, const auto& b) { return std::format("{}{}{}", a, delimiter, b); }
  );
}

///
/// Check if string_view starts with prefix.
/// \return true if string_view starts with prefix, false otherwise
///
constexpr auto starts_with(const std::string_view data, const std::string_view prefix) -> bool
{
  if(prefix.empty())
  {
    throw util::exception{"prefix cannot be empty"};
  }
  return prefix.size() <= data.size() && std::equal(std::cbegin(prefix), std::cend(prefix), std::cbegin(data));
}

///
/// Check if string_view ends with postfix.
/// \return true if string_view ends with postfix, false otherwise
///
constexpr auto ends_with(const std::string_view data, const std::string_view postfix) -> bool
{
  if(postfix.empty())
  {
    throw util::exception{"postfix cannot be empty"};
  }
  return postfix.size() <= data.size() &&
         std::equal(std::cbegin(postfix), std::cend(postfix), std::prev(std::cend(data), postfix.size()));
}

///
/// Check if string_view starts with prefix.
/// \return true if string_view starts with char, false otherwise
///
constexpr auto starts_with(const std::string_view data, const char prefix_char) -> bool
{
  return !data.empty() && data.front() == prefix_char;
}

///
/// Check if string_view ends with postfix.
/// \return true if string_view ends with char, false otherwise
///
constexpr auto ends_with(const std::string_view data, const char postfix_char) -> bool
{
  return !data.empty() && data.back() == postfix_char;
}

///
/// Check if string_view ends with a formatted number (e.g., " (1)", " [2], or other").
/// In \p format '{}' stands for the number, e.g. " ({})" or " [{}]".
/// \return true if string_view ends with a formatted number, false otherwise
///
constexpr auto ends_with_formatted_uint(const std::string_view data, const std::string_view format = "")
  -> std::optional<std::uint32_t>
{
  static constexpr std::string_view placeholder = "{}";
  const auto placeholder_pos = format.rfind(placeholder);
  if(placeholder_pos == std::string_view::npos || format.empty() || data.size() < format.size())
  {
    return std::nullopt;
  }

  const auto format_prefix = format.substr(0, placeholder_pos);
  const auto format_suffix = format.substr(placeholder_pos + placeholder.size());
  if(!format_suffix.empty() && !ends_with(data, format_suffix))
  {
    return std::nullopt;
  }
  const auto suffix_pos = data.size() - format_suffix.size();
  const auto prefix_pos = [&]
  {
    if(format_prefix.empty())
    {
      const auto data_without_suffix = data.substr(0, suffix_pos);
      static constexpr auto is_digit = [](const unsigned char c) { return std::isdigit(c); };
      auto reverse = data_without_suffix | std::views::reverse | std::views::take_while(is_digit);
      const auto digit_count = std::ranges::count(reverse, std::size_t{0});
      return digit_count == 0 ? std::string_view::npos : suffix_pos - digit_count;
    }
    else
    {
      return data.rfind(format_prefix);
    }
  }();
  if(prefix_pos != std::string_view::npos && prefix_pos < suffix_pos)
  {
    const auto number_start = prefix_pos + format_prefix.size();
    if(number_start < data.size() && suffix_pos != std::string_view::npos && suffix_pos > number_start)
    {
      const auto number_size = suffix_pos - number_start;
      if(
        const auto result = txt::find_uint(data.substr(number_start, number_size));
        result.has_value() && result->post_value_offset == number_size
      )
      {
        return result->value;
      }
    }
  }
  return std::nullopt;
}

} // namespace bibstd::util::string
