#include "bibstd/bible/scripture_types.hpp"
#include "bibstd/util/const_map.hpp"

#include <algorithm>
#include <format>

namespace bibstd::bible
{

///
///
auto passage_markup::escaped_xml(const std::string_view text) -> std::string
{
  static constexpr auto entities = util::make_const_map<char, std::string_view>({
    {'&', "&amp;"},
    {'<',  "&lt;"},
    {'>',  "&gt;"}
  });
  auto result = std::string{};
  result.reserve(text.size());
  std::ranges::for_each(
    text,
    [&result](const auto character)
    {
      if(entities.contains(character))
      {
        result.append(entities.at(character));
      }
      else
      {
        result.push_back(character);
      }
    }
  );
  return result;
}

///
///
auto passage_markup::section(const std::string_view position, const std::string_view content) -> std::string
{
  return std::format(R"(<{0} {1}="{2}">{3}</{0}>)", paragraph, paragraph_attribute, position, content);
}

} // namespace bibstd::bible
