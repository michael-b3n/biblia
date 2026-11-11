#include "bibstd/bible/ocr_book_variants.hpp"
#include "arcstd/math/combination.hpp"
#include "arcstd/txt/script_common.hpp"
#include "arcstd/txt/script_letters.hpp"

#include <string>

namespace bibstd::bible
{

///
///
auto ocr_book_variants::name_variant_aliases(const std::string_view name_variant, const util::language language)
  -> std::vector<std::string>
{
  auto char_variants = std::vector<std::vector<std::string_view>>{};
  txt::script_letters::visit(
    language,
    [&](const auto& letters)
    {
      txt::script_common::for_each_char(
        letters,
        name_variant,
        [&](const auto character, [[maybe_unused]] const auto pos, [[maybe_unused]] const auto category)
        {
          auto variants = std::vector<std::string_view>{character};
          std::ranges::for_each(
            ocr_char_alias_map | std::views::filter([&](const auto pair) { return pair.second == character; }),
            [&](const auto pair) { variants.push_back(pair.first); }
          );
          char_variants.push_back(std::move(variants));
        }
      );
    }
  );

  auto result = std::vector<std::string>{};
  math::for_each_combination(
    char_variants,
    [&](const auto& combination)
    {
      auto alias = std::string{};
      alias.reserve(name_variant.size());
      std::ranges::for_each(combination, [&](const auto character) { alias.append(character); });
      // Every combination of the char variants is an alias, except the name variant itself.
      if(alias != name_variant)
      {
        result.push_back(std::move(alias));
      }
      return true;
    }
  );
  return result;
}

///
///
auto ocr_book_variants::name_variants_with_aliases(util::language language) -> std::span<const std::pair<book_id, std::string>>
{
  const auto create = [](const util::language language)
  {
    auto result = std::vector<std::pair<book_id, std::string>>{};
    std::ranges::for_each(
      name_variants_span(language),
      [&](const auto& element)
      {
        const auto& [book_id, name_variant] = element;
        const auto aliases = name_variant_aliases(name_variant, language);
        result.reserve(result.size() + aliases.size() + 1);
        result.emplace_back(book_id, std::string{name_variant});
        std::ranges::for_each(aliases, [&](const auto& alias) { result.emplace_back(book_id, alias); });
      }
    );
    return result;
  };

  static const auto name_variants_with_aliases_de = create(util::language::german);
  static const auto name_variants_with_aliases_en = create(util::language::english);
  switch(language)
  {
  case util::language::german: return name_variants_with_aliases_de;
  case util::language::english: return name_variants_with_aliases_en;
  default: throw util::exception{"unsupported language"}; return {};
  }
}

} // namespace bibstd::bible
