#pragma once

#include "bibstd/bible/ocr_book_variants_de.hpp"
#include "bibstd/bible/ocr_book_variants_en.hpp"
#include "bibstd/util/const_map.hpp"
#include "bibstd/util/language.hpp"
#include "bibstd/util/ranges.hpp"

#include <algorithm>
#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace bibstd::bible
{

///
/// Holds all bible book name variants corresponding to book id, per supported language.
///
struct ocr_book_variants final
{
private: // Constants
  ///
  /// A map of common misread characters by OCR to their correct character.
  ///
  static constexpr auto ocr_char_alias_map = util::make_const_map<std::string_view, std::string_view>({
    // Misread char         Correct char
    {std::string_view{"0"}, std::string_view{"o"}},
    {std::string_view{"I"}, std::string_view{"1"}},
    {std::string_view{"l"}, std::string_view{"1"}},
    {std::string_view{"m"}, std::string_view{"M"}},
    {std::string_view{"O"}, std::string_view{"o"}},
    {std::string_view{"s"}, std::string_view{"S"}},
    {std::string_view{"S"}, std::string_view{"s"}},
  });

  ///
  /// Concatenated list of all bible book name variants.
  ///
  static constexpr auto name_variants_list = [](const auto& name_variants)
  {
    static constexpr auto get = []<std::size_t I>(const auto& name_variants)
    {
      const auto element = std::get<I>(name_variants);
      constexpr auto size = std::tuple_size_v<decltype(element.second)>;
      std::array<std::pair<book_id, std::string_view>, size> result;
      std::ranges::for_each(
        util::ranges::index_view(result), [&](const auto i) { result.at(i) = std::pair{element.first, element.second.at(i)}; }
      );
      return result;
    };
    static constexpr auto to_array = [](auto&& tuple)
    {
      static constexpr auto get_array = [](auto&&... e) { return std::array{std::forward<decltype(e)>(e)...}; };
      return std::apply(get_array, std::forward<decltype(tuple)>(tuple));
    };
    return []<std::size_t... I>(std::index_sequence<I...>, const auto& name_variants)
    {
      return to_array(std::tuple_cat(get.template operator()<I>(name_variants)...));
    }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<decltype(name_variants)>>>{}, name_variants);
  };

public: // Constants
  static constexpr auto name_variants_list_de = name_variants_list(ocr_book_variants_de::name_variants);
  static constexpr auto name_variants_list_en = name_variants_list(ocr_book_variants_en::name_variants);

  // Static Methods
  ///
  /// \return the span of all name variants based on language.
  ///
  [[nodiscard]] static constexpr auto name_variants_span(util::language language) -> std::span<const std::pair<book_id, std::string_view>>;

  ///
  /// \return a list of name variants that are aliases of the given name variant.
  ///
  [[nodiscard]] static auto name_variant_aliases(std::string_view name_variant, util::language language) -> std::vector<std::string>;

  ///
  /// \return a complete list of all name variants with their aliases based on language.
  ///
  [[nodiscard]] static auto name_variants_with_aliases(util::language language) -> std::span<const std::pair<book_id, std::string>>;
};

///
///
constexpr auto ocr_book_variants::name_variants_span(const util::language language)
  -> std::span<const std::pair<book_id, std::string_view>>
{
  using result_type = std::span<const std::pair<book_id, std::string_view>>;
  // clang-format off
  switch(language)
  {
  case util::language::german: return result_type{name_variants_list_de};
  case util::language::english: return result_type{name_variants_list_en};
  default: throw util::exception{"unsupported language"}; return result_type{};
  }
  // clang-format on
}

} // namespace bibstd::bible
