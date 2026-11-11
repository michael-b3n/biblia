#pragma once

#include "arcstd/math/value_range.hpp"
#include "arcstd/util/language.hpp"
#include "bibstd/bible/common.hpp"
#include "bibstd/bible/reference_range.hpp"
#include "bibstd/bible/versification.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace bibstd::bible
{

///
/// This class searches strings and identifies bible references.
///
struct reference_parser final
{
private: // Typedefs
  ///
  /// Passage numbers and the transition chars separating them, in the order they were read.
  ///
  using passage_template_value_type = std::variant<std::uint32_t, char>;
  using passage_template_type = std::vector<passage_template_value_type>;

  enum class passage_level
  {
    chapter,
    verse,
  };

  struct find_book_result final
  {
    book_id book;
    arcstd::math::value_range<std::size_t> index_range_book;
    arcstd::math::value_range<std::size_t> index_range_numbers;
    std::string_view book_name_variant;
  };

  struct passage_section final
  {
    std::vector<std::uint32_t> numbers;
    std::string generic_template;
  };

  struct normalized_passage final
  {
    std::string text;
    std::vector<arcstd::math::value_range<std::size_t>> raw_index_ranges;
  };

  struct passage_template_result final
  {
    passage_template_type passage_template;
    std::size_t index_numbers_end{0};
  };

private: // Constants
  ///
  /// List of all characters which can be used to separate passage numbers.
  ///
  static constexpr auto transition_chars = std::array{',', ';', '.', '-', ':', '&', '+'};

  ///
  /// List of all postfixes which can appear in a bible reference after a number.
  /// \note Adjust `number_postfix_chars` if more postfixes are added.
  ///
  static constexpr auto number_postfixes = std::array{
    std::string_view("ff."),
    std::string_view("f."),
    std::string_view("a"),
    std::string_view("b"),
    std::string_view("c"),
    std::string_view("d")
  };

  ///
  /// List of all letters appearing in `number_postfixes`.
  ///
  static constexpr auto number_postfix_chars = std::array{'f', 'a', 'b', 'c', 'd'};

  ///
  /// Character the passage normalization emits for text that cannot be part of a reference.
  /// It is no transition char, so it ends the passage template unless a transition char bridges it.
  ///
  static constexpr auto gap = ' ';

  ///
  /// Transition char that also ends a sentence, \see skip_gap.
  ///
  static constexpr auto fullstop_char = '.';

public: // Typedefs
  using index_range_type = arcstd::math::value_range<std::size_t>;

  ///
  /// Parsed reference ranges together with the index range of the text they originate from.
  ///
  struct parse_result final
  {
    std::vector<reference_range> ranges;
    index_range_type index_range_origin{0u, 0u};
  };

public: // Operations
  ///
  /// Parse the bible reference located at the given index. Only references of a single book are parsed.
  /// \return Reference ranges and the index range of the text they originate from
  ///
  [[nodiscard]] static auto parse(
    std::string_view text, std::size_t index, arcstd::util::language language, const versification& versification
  ) -> parse_result;

private: // Implementation
  ///
  /// Find the book name variant surrounding the given index.
  /// \return Book and the index ranges of its name and of the numbers following it, std::nullopt if there is no book
  ///
  [[nodiscard]] static auto find_book(std::string_view text, std::size_t index, arcstd::util::language language)
    -> std::optional<find_book_result>;

  ///
  /// Find the text of the reference at \p index, from its book name up to the last number its passage template consumed.
  /// \return Index range of the reference text, std::nullopt if \p index lies behind the word the reference ends in
  ///
  [[nodiscard]] static auto reference_index_range(
    std::string_view text,
    std::size_t index,
    const find_book_result& book,
    const passage_template_result& passage_template,
    arcstd::util::language language
  ) -> std::optional<index_range_type>;

  ///
  /// Find the end of the word the character at \p index belongs to.
  /// \return index of the whitespace behind the word, the size of \p text if no whitespace follows
  ///
  [[nodiscard]] static auto end_of_word(std::string_view text, std::size_t index, arcstd::util::language language)
    -> std::size_t;

  ///
  /// Find the numbers after the book name that are possibly part of the reference. They are not guaranteed to be,
  /// the numbers might belong to another book or to a list.
  /// \return Index behind the last number, std::nullopt if there is no number
  ///
  [[nodiscard]] static auto find_numbers_after_book_name(std::string_view text_after_name, arcstd::util::language language)
    -> std::optional<std::size_t>;

  ///
  /// The last number of the numbers range may already belong to the next book name, e.g. the "1" of "1.Mose".
  /// \return Numbers range without such a number
  ///
  [[nodiscard]] static auto try_validate_numbers_range(
    std::string_view text_after_name, std::size_t numbers_end, arcstd::util::language language
  ) -> std::size_t;

  ///
  /// Create a passage template from the passage text. The numbers found by \see find_numbers_after_book_name may reach
  /// beyond the reference, so the template also reports how much of the passage text it actually consumed.
  /// \return Passage template and the consumed part of the passage text
  ///
  [[nodiscard]] static auto create_passage_template(std::string_view passage_text, arcstd::util::language language)
    -> passage_template_result;

  ///
  /// Replace everything that cannot be part of a passage template with a gap. The gap keeps numbers apart that are
  /// separated by text or by a line break.
  /// \return Normalized text with every character mapped back to its index range in the original text
  ///
  [[nodiscard]] static auto normalize_passage_text(std::string_view text, arcstd::util::language language)
    -> normalized_passage;

  ///
  /// Identify the number at the given index and move the index behind it.
  /// \return Number at the given index, std::nullopt if there is none
  ///
  [[nodiscard]] static auto identify_number(std::string_view text, std::size_t& pos) -> std::optional<std::uint32_t>;

  ///
  /// Identify the transition character at the given index and move the index behind it.
  /// \return Transition character at the given index, std::nullopt if there is none
  ///
  [[nodiscard]] static auto identify_transition(std::string_view text, std::size_t& pos) -> std::optional<char>;

  ///
  /// Skip the gap at the given index and move the index behind it. A gap still belongs to the reference while a
  /// transition character is pending or while another one follows it, so that a spaced out or line broken reference
  /// stays readable. A gap after a full stop ends a sentence rather than separating passage numbers.
  /// \return false if the gap ends the reference, true otherwise
  ///
  [[nodiscard]] static auto skip_gap(
    std::string_view text, std::size_t& pos, const passage_template_type& current_passage_template
  ) -> bool;

  ///
  /// Match a passage template against the versification. Which character separates chapter and verse is unknown, so
  /// every candidate is matched and the interpretation covering the fewest verses is taken.
  /// \return Reference ranges matching the passage template
  ///
  [[nodiscard]] static auto match_passage_template(
    book_id book, const passage_template_type& passage_template, const versification& versification
  ) -> std::vector<reference_range>;

  ///
  /// \return Transition characters of the passage template, without duplicates and without '-'
  ///
  [[nodiscard]] static auto passage_template_transition_chars(const passage_template_type& passage_template)
    -> std::vector<char>;

  ///
  /// \return Numbers of the passage template
  ///
  [[nodiscard]] static auto passage_template_numbers(const passage_template_type& passage_template)
    -> std::vector<std::uint32_t>;

  ///
  /// Split the passage template into sections at every transition character other than the given one and '-'.
  /// \return Passage sections
  ///
  [[nodiscard]] static auto create_passage_sections(
    const passage_template_type& passage_template, std::optional<char> down_transition_char
  ) -> std::vector<passage_section>;
};

} // namespace bibstd::bible
