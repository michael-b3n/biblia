#include "bibstd/bible/scripture_types.hpp"

#include <arcstd/util/const_map.hpp>
#include <arcstd/util/exception.hpp>
#include <arcstd/util/format.hpp>
#include <arcstd/util/log.hpp>

#include <pugixml.hpp>

#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bibstd::bible
{

///
///
auto passage_markup::escaped_xml(const std::string_view text) -> std::string
{
  static constexpr auto entities = arcstd::util::make_const_map<char, std::string_view>({
    {'&', "&amp;"},
    {'<',  "&lt;"},
    {'>',  "&gt;"}
  });
  const auto escaped = [](const char& character)
  { return entities.contains(character) ? entities.at(character) : std::string_view{&character, 1}; };
  return text | std::views::transform(escaped) | std::views::join | std::ranges::to<std::string>();
}

///
///
auto passage_markup::section(const std::string_view position, const std::string_view content) -> std::string
{
  return std::format(R"(<{0} {1}="{2}">{3}</{0}>)", paragraph, paragraph_attribute, position, content);
}

///
///
auto passage_markup::to_xml(const std::string_view plain) -> std::string
{
  return section(paragraph_undefined, escaped_xml(plain));
}

///
///
auto passage_markup::to_plain(const std::string_view xml) -> std::string
{
  // A verse spanning several paragraphs has several roots, whitespace may be all there is between two nodes
  static constexpr auto parse_options = pugi::parse_default | pugi::parse_fragment | pugi::parse_ws_pcdata;
  auto document = pugi::xml_document{};
  if(!document.load_buffer(xml.data(), xml.size(), parse_options, pugi::encoding_utf8))
  {
    static constexpr auto shorten = [](const std::string_view text, const std::size_t length) -> std::string
    { return text.size() <= length ? std::string{text} : std::format("{}...", text.substr(0, length)); };
    LOG_WARN("failed to parse XML of a passage: xml=\"{}\"", shorten(xml, 80));
    return std::string{xml};
  }
  static const auto normalized_text = pugi::xpath_query{"normalize-space()"};
  const auto text_of = [](const pugi::xml_node& section) { return normalized_text.evaluate_string(section); };
  const auto filled = [](const std::string& text) { return !text.empty(); };
  const auto sections =
    document.children() | std::views::transform(text_of) | std::views::filter(filled) | std::ranges::to<std::vector>();
  return arcstd::util::format::join(sections, " ");
}

///
///
passage::passage(reference ref, std::string content, const passage_format format)
  : ref_{ref}
  , content_{std::move(content)}
  , format_{format}
{
}

///
///
auto passage::ref() const -> const reference&
{
  return ref_;
}

///
///
auto passage::cross_references() const -> const std::vector<std::string>&
{
  return cross_references_;
}

///
///
auto passage::xml() const -> std::string
{
  switch(format_)
  {
  case passage_format::xml: return content_;
  case passage_format::plain: return passage_markup::to_xml(content_);
  default: throw arcstd::util::exception{"unsupported passage format"};
  }
}

///
///
auto passage::plain() const -> std::string
{
  switch(format_)
  {
  case passage_format::xml: return passage_markup::to_plain(content_);
  case passage_format::plain: return content_;
  default: throw arcstd::util::exception{"unsupported passage format"};
  }
}

///
///
auto passage::cross_references(std::vector<std::string> cross_references) -> void
{
  cross_references_ = std::move(cross_references);
}

} // namespace bibstd::bible
