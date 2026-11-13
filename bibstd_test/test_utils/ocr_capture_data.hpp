#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::test_utils
{

///
/// Bounding box of a captured OCR element, in the coordinate system of the captured image.
///
struct capture_box final
{
  std::int32_t x{0};
  std::int32_t y{0};
  std::int32_t width{0};
  std::int32_t height{0};

  auto operator==(const capture_box&) const -> bool = default;
};

///
/// Text of a captured OCR element together with its bounding box.
///
struct capture_text final
{
  std::string text;
  capture_box box;

  auto operator==(const capture_text&) const -> bool = default;
};

///
/// Captured word with the line and the paragraph it belongs to, if the engine reported one.
///
struct capture_word final
{
  capture_text word;
  std::optional<capture_text> line;
  std::optional<capture_text> paragraph;
};

///
/// Captured layout analysis element.
///
struct capture_layout final
{
  capture_box line;
  std::optional<capture_box> paragraph;
};

///
/// Everything a real OCR engine reported for one image, enough to replay it without the image.
///
struct capture_data final
{
  std::string id;
  std::uint32_t width{0};
  std::uint32_t height{0};
  std::vector<capture_layout> layouts;
  std::vector<capture_word> words;
};

//
// The capture file holds one record per line, each opened by a tag:
//
//   image  <width> <height>
//   text   <box> <text>
//   word   <box> <line index|-> <paragraph index|-> <text>
//   layout <box> <box|- - - ->
//
// A box is the four numbers `x y width height`. Words refer to the text records by index, so that
// the paragraph text is not repeated for each of its words.
//

namespace detail
{

///
/// Placeholder standing in for an index or a box that was not reported.
///
constexpr auto none = std::string_view{"-"};

///
/// \return Text with backslashes and line breaks escaped, so that it fits on one line of the capture file
///
inline auto escape(const std::string_view text) -> std::string
{
  auto result = std::string{};
  for(const auto c : text)
  {
    switch(c)
    {
    case '\\': result += "\\\\"; break;
    case '\n': result += "\\n"; break;
    case '\r': result += "\\r"; break;
    default: result += c; break;
    }
  }
  return result;
}

///
/// \return Text with the escaping of \see escape undone
///
inline auto unescape(const std::string_view text) -> std::string
{
  auto result = std::string{};
  for(auto i = std::size_t{0}; i < text.size(); ++i)
  {
    if(text.at(i) != '\\' || i + 1 == text.size())
    {
      result += text.at(i);
      continue;
    }
    ++i;
    switch(text.at(i))
    {
    case 'n': result += '\n'; break;
    case 'r': result += '\r'; break;
    default: result += text.at(i); break;
    }
  }
  return result;
}

///
/// \return Bounding box as the four numbers of one capture file record
///
inline auto to_record(const capture_box& box) -> std::string
{
  return std::format("{} {} {} {}", box.x, box.y, box.width, box.height);
}

///
/// Take the next space separated token off the record and move the record behind it.
/// \return Token, empty when the record is exhausted
///
inline auto take_token(std::string_view& record) -> std::string_view
{
  const auto end = std::min(record.find(' '), record.size());
  const auto result = record.substr(0, end);
  record.remove_prefix(std::min(end + 1, record.size()));
  return result;
}

///
/// Take the remainder of the record, which is the text of a record that carries one.
/// \return Unescaped text
///
inline auto take_text(std::string_view& record) -> std::string
{
  const auto result = unescape(record);
  record = std::string_view{};
  return result;
}

///
/// \return Number the token holds, std::nullopt when the token is no number
///
template<typename T>
auto to_number(const std::string_view token) -> std::optional<T>
{
  auto result = T{};
  const auto* const begin = std::to_address(std::ranges::cbegin(token));
  const auto* const end = std::to_address(std::ranges::cend(token));
  const auto [parsed, error] = std::from_chars(begin, end, result);
  return error == std::errc{} && parsed == end ? std::optional{result} : std::nullopt;
}

///
/// Take the four numbers of a bounding box off the record.
/// \return Bounding box, std::nullopt when a number is missing or malformed
///
inline auto take_box(std::string_view& record) -> std::optional<capture_box>
{
  auto result = capture_box{};
  for(auto* const value : {&result.x, &result.y, &result.width, &result.height})
  {
    const auto number = to_number<std::int32_t>(take_token(record));
    if(!number)
    {
      return std::nullopt;
    }
    *value = *number;
  }
  return result;
}

} // namespace detail

///
/// Write the captured data of one image, \see the format description above.
/// \return false when the file cannot be written
///
inline auto write_capture(const std::filesystem::path& path, const capture_data& data) -> bool
{
  auto file = std::ofstream{path, std::ios::binary};
  if(!file)
  {
    return false;
  }
  auto texts = std::vector<capture_text>{};
  const auto index_of = [&](const std::optional<capture_text>& text) -> std::string
  {
    if(!text)
    {
      return std::string{detail::none};
    }
    const auto it = std::ranges::find(texts, *text);
    if(it == std::ranges::cend(texts))
    {
      texts.push_back(*text);
      return std::format("{}", texts.size() - 1);
    }
    return std::format("{}", std::ranges::distance(std::ranges::cbegin(texts), it));
  };

  // The text table is filled while the records are formatted, so the records are held back until it is complete.
  auto records = std::string{};
  for(const auto& word : data.words)
  {
    // Both indices are taken before formatting, because index_of appends to the text table and the
    // order in which format evaluates its arguments is unspecified.
    const auto line_index = index_of(word.line);
    const auto paragraph_index = index_of(word.paragraph);
    records += std::format(
      "word {} {} {} {}\n", detail::to_record(word.word.box), line_index, paragraph_index, detail::escape(word.word.text)
    );
  }
  for(const auto& layout : data.layouts)
  {
    records += std::format(
      "layout {} {}\n",
      detail::to_record(layout.line),
      layout.paragraph ? detail::to_record(*layout.paragraph) : std::format("{0} {0} {0} {0}", detail::none)
    );
  }

  file << std::format("image {} {}\n", data.width, data.height);
  for(const auto& text : texts)
  {
    file << std::format("text {} {}\n", detail::to_record(text.box), detail::escape(text.text));
  }
  file << records;
  return true;
}

///
/// \return Captured data read back from a file written by \see write_capture, std::nullopt on a broken file
///
inline auto read_capture(const std::filesystem::path& path) -> std::optional<capture_data>
{
  auto file = std::ifstream{path, std::ios::binary};
  if(!file)
  {
    return std::nullopt;
  }
  auto result = capture_data{.id = path.stem().string()};
  auto texts = std::vector<capture_text>{};
  auto line = std::string{};
  while(std::getline(file, line))
  {
    if(!line.empty() && line.back() == '\r')
    {
      line.pop_back();
    }
    auto record = std::string_view{line};
    const auto tag = detail::take_token(record);

    // A capture killed mid write leaves a record without its numbers. Such a half written capture is
    // a broken file like any other, so every incomplete record rejects the whole file.
    if(tag == std::string_view{"image"})
    {
      const auto width = detail::to_number<std::uint32_t>(detail::take_token(record));
      const auto height = detail::to_number<std::uint32_t>(detail::take_token(record));
      if(!width || !height)
      {
        return std::nullopt;
      }
      result.width = *width;
      result.height = *height;
    }
    else if(tag == std::string_view{"text"})
    {
      const auto box = detail::take_box(record);
      if(!box)
      {
        return std::nullopt;
      }
      texts.emplace_back(capture_text{.text = detail::take_text(record), .box = *box});
    }
    else if(tag == std::string_view{"word"})
    {
      const auto box = detail::take_box(record);
      if(!box)
      {
        return std::nullopt;
      }
      const auto at = [&](const std::string_view token) -> std::optional<capture_text>
      {
        const auto index = detail::to_number<std::size_t>(token);
        return index && *index < texts.size() ? std::optional{texts.at(*index)} : std::nullopt;
      };
      const auto line_index = detail::take_token(record);
      const auto paragraph_index = detail::take_token(record);
      const auto reported_or_none = [](const std::string_view token)
      { return token == detail::none || detail::to_number<std::size_t>(token).has_value(); };
      if(!reported_or_none(line_index) || !reported_or_none(paragraph_index))
      {
        return std::nullopt;
      }
      const auto text_line = at(line_index);
      const auto text_paragraph = at(paragraph_index);
      result.words.emplace_back(
        capture_word{
          .word = capture_text{.text = detail::take_text(record), .box = *box},
            .line = text_line, .paragraph = text_paragraph
      }
      );
    }
    else if(tag == std::string_view{"layout"})
    {
      const auto box = detail::take_box(record);
      if(!box)
      {
        return std::nullopt;
      }
      const auto reported = !record.starts_with(detail::none);
      const auto paragraph = reported ? detail::take_box(record) : std::optional<capture_box>{};
      if(reported && !paragraph)
      {
        return std::nullopt;
      }
      result.layouts.emplace_back(capture_layout{.line = *box, .paragraph = paragraph});
    }
  }
  return result.width > 0 ? std::optional{result} : std::nullopt;
}

///
/// \return Captured data of every capture file in the given folder, empty when there is none
///
inline auto read_captures(const std::filesystem::path& folder) -> std::vector<capture_data>
{
  auto result = std::vector<capture_data>{};
  if(!std::filesystem::is_directory(folder))
  {
    return result;
  }
  for(const auto& entry : std::filesystem::directory_iterator{folder})
  {
    if(entry.path().extension() == std::filesystem::path{".ocr"})
    {
      if(auto data = read_capture(entry.path()))
      {
        result.emplace_back(std::move(*data));
      }
    }
  }
  std::ranges::sort(result, [](const auto& a, const auto& b) { return a.id < b.id; });
  return result;
}

} // namespace bibstd::test_utils
