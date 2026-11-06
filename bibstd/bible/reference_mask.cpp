#include "bibstd/bible/reference_mask.hpp"
#include "bibstd/util/string.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace bibstd::bible
{
namespace
{

// Typedefs
using values_type = reference_mask::values;

///
/// Variable of a mask, e.g. `[book]`, and how its value is read.
///
struct variable final
{
  std::string_view key;
  auto (*value)(const values_type&) -> std::string;
};

///
/// Character written by its key, e.g. `[nbsp]`.
///
struct character final
{
  std::string_view key;
  std::string_view value;
};

///
/// Rendered part of a mask, its flags decide whether the group around it drops out.
///
struct rendered final
{
  std::string text;
  bool has_variable{false};
  bool has_value{false};
};

// Constants
constexpr auto group_begin = '{';
constexpr auto group_end = '}';
constexpr auto variable_begin = '[';
constexpr auto variable_end = ']';

///
/// \return the chapter at the begin of the range
///
[[nodiscard]] auto chapter_begin(const values_type& values) -> std::string
{
  return std::format("{}", values.range.begin().chapter());
}

///
/// \return the verse at the begin of the range
///
[[nodiscard]] auto verse_begin(const values_type& values) -> std::string
{
  return std::format("{}", values.range.begin().verse());
}

///
/// \return the book at the end of the range, empty if it repeats the begin
///
[[nodiscard]] auto book_end(const values_type& values) -> std::string
{
  return values.range.begin().book() != values.range.end().book() ? values.book_end : std::string{};
}

///
/// \return the chapter at the end of the range, empty if it repeats the begin
///
[[nodiscard]] auto chapter_end(const values_type& values) -> std::string
{
  const auto begin = values.range.begin();
  const auto end = values.range.end();
  return begin.book() != end.book() || begin.chapter() != end.chapter() ? std::format("{}", end.chapter()) : std::string{};
}

///
/// \return the verse at the end of the range, empty if it repeats the begin
///
[[nodiscard]] auto verse_end(const values_type& values) -> std::string
{
  return values.range.begin() != values.range.end() ? std::format("{}", values.range.end().verse()) : std::string{};
}

// clang-format off
constexpr auto variables = std::array{
  variable{.key = "content",          .value = [](const values_type& values) { return values.content; }},
  variable{.key = "content_numbered", .value = [](const values_type& values) { return values.content_numbered; }},
  variable{.key = "book",             .value = [](const values_type& values) { return values.book; }},
  variable{.key = "book_long",        .value = [](const values_type& values) { return values.book_long; }},
  variable{.key = "book_abbr",        .value = [](const values_type& values) { return values.book_abbreviation; }},
  variable{.key = "book_end",         .value = book_end},
  variable{.key = "#chapter_begin",   .value = chapter_begin},
  variable{.key = "#verse_begin",     .value = verse_begin},
  variable{.key = "#chapter_end",     .value = chapter_end},
  variable{.key = "#verse_end",       .value = verse_end},
  variable{.key = "scripture",        .value = [](const values_type& values) { return values.scripture; }},
  variable{.key = "scripture_abbr",   .value = [](const values_type& values) { return values.scripture_abbreviation; }},
  variable{.key = "copyright",        .value = [](const values_type& values) { return values.copyright; }},
};

constexpr auto characters = std::array{
  character{.key = "nbsp",    .value = "\u00A0"},
  character{.key = "newline", .value = "\n"},
  character{.key = "tab",     .value = "\t"},
};
// clang-format on

///
/// \return position of the brace closing the group that opens at \p begin, std::string_view::npos if none does
///
[[nodiscard]] auto closing_brace(const std::string_view mask, const std::size_t begin) -> std::size_t
{
  auto depth = std::size_t{0};
  const auto closes_group = [&](const std::size_t position)
  {
    if(mask.at(position) == group_begin)
    {
      ++depth;
      return false;
    }
    return mask.at(position) == group_end && --depth == 0;
  };
  const auto positions = std::views::iota(begin, mask.size());
  const auto found = std::ranges::find_if(positions, closes_group);
  return found != std::ranges::cend(positions) ? *found : std::string_view::npos;
}

///
/// \return the entry of \p entries with \p key, the end of \p entries if there is none
///
[[nodiscard]] auto find_key(const auto& entries, const std::string_view key)
{
  return std::ranges::find_if(entries, [key](const auto& entry) { return entry.key == key; });
}

///
/// \return \p first followed by \p second
///
[[nodiscard]] auto joined(rendered first, const rendered& second) -> rendered
{
  first.text.append(second.text);
  first.has_variable = first.has_variable || second.has_variable;
  first.has_value = first.has_value || second.has_value;
  return first;
}

///
/// \return \p group in the text around it: as typed without variables, dropped if all of them are empty
///
[[nodiscard]] auto in_text(const rendered& group) -> rendered
{
  if(!group.has_variable)
  {
    return rendered{.text = std::format("{}{}{}", group_begin, group.text, group_end)};
  }
  return group.has_value ? group : rendered{.has_variable = true};
}

[[nodiscard]] auto render_part(std::string_view mask, const values_type& values) -> rendered;

///
/// \return the first piece of \p mask rendered, a group, a key or text up to the next one, and its size
///
[[nodiscard]] auto render_first(const std::string_view mask, const values_type& values) -> std::pair<rendered, std::size_t>
{
  if(util::string::starts_with(mask, group_begin))
  {
    if(const auto end = closing_brace(mask, 0); end != std::string_view::npos)
    {
      return {in_text(render_part(mask.substr(1, end - 1), values)), end + 1};
    }
  }
  else if(util::string::starts_with(mask, variable_begin))
  {
    if(const auto end = mask.find(variable_end); end != std::string_view::npos)
    {
      const auto name = mask.substr(1, end - 1);
      if(const auto found = find_key(variables, name); found != std::ranges::cend(variables))
      {
        auto value = found->value(values);
        const auto has_value = !value.empty();
        return {
          rendered{.text = std::move(value), .has_variable = true, .has_value = has_value},
          end + 1
        };
      }
      if(const auto found = find_key(characters, name); found != std::ranges::cend(characters))
      {
        return {rendered{.text = std::string{found->value}}, end + 1};
      }
    }
  }
  // An opening char that opens nothing is text too
  static constexpr auto openings = std::array{group_begin, variable_begin};
  const auto size = std::min(mask.find_first_of(std::string_view{openings.data(), openings.size()}, 1), mask.size());
  return {rendered{.text = std::string{mask.substr(0, size)}}, size};
}

///
/// \return \p mask rendered, and whether it holds variables and values, which decides on the group it is in
///
auto render_part(const std::string_view mask, const values_type& values) -> rendered
{
  if(mask.empty())
  {
    return rendered{};
  }
  const auto [first, size] = render_first(mask, values);
  return joined(first, render_part(mask.substr(size), values));
}

} // namespace

///
///
auto reference_mask::render(const std::string_view mask, const values& values) -> std::string
{
  return render_part(mask, values).text;
}

///
///
auto reference_mask::uses_content(const std::string_view mask) -> bool
{
  static constexpr auto names = util::string::to_string_view_array("[content]", "[content_numbered]");
  return std::ranges::any_of(names, [mask](const std::string_view name) { return mask.contains(name); });
}

} // namespace bibstd::bible
