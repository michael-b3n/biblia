#include "bibstd/bible/reference_parser.hpp"
#include "bibstd/bible/common.hpp"
#include "bibstd/bible/ocr_book_variants.hpp"
#include "bibstd/bible/reference.hpp"
#include "bibstd/bible/versification.hpp"
#include "bibstd/math/value_range.hpp"
#include "bibstd/txt/find_uint.hpp"
#include "bibstd/txt/script_common.hpp"
#include "bibstd/txt/script_letters.hpp"
#include "bibstd/util/contains.hpp"
#include "bibstd/util/language.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/ranges.hpp"
#include "bibstd/util/string.hpp"
#include "bibstd/util/visit_helper.hpp"

#include <algorithm>
#include <cassert>
#include <map>
#include <ranges>

namespace bibstd::bible
{
namespace
{

///
/// Match one section of a passage template.
/// \return Reference ranges matching the section
///
auto match_passage_template_section(
  const book_id book,
  const std::span<const std::uint32_t> numbers,
  const std::string_view section,
  auto& current_level,
  std::uint32_t& current_chapter,
  const versification& versification
) -> std::vector<reference_range>
{
  using passage_level = std::remove_reference_t<decltype(current_level)>;
  auto result = std::vector<reference_range>{};
  const auto numbers_size = numbers.size();
  if(std::string_view("#X#-#X#") == section && numbers_size == 4)
  {
    const auto ref1 = reference::create(book, numbers.at(0), numbers.at(1), versification);
    const auto ref2 = reference::create(book, numbers.at(2), numbers.at(3), versification);
    if(ref1 && ref2)
    {
      result.emplace_back(ref1.value(), ref2.value());
    }
    current_level = passage_level::verse;
    current_chapter = numbers.at(2);
  }
  else if(std::string_view("#X#-#") == section && numbers_size == 3)
  {
    const auto ref1 = reference::create(book, numbers.at(0), numbers.at(1), versification);
    const auto ref2 = reference::create(book, numbers.at(0), numbers.at(2), versification);
    if(ref1 && ref2)
    {
      result.emplace_back(ref1.value(), ref2.value());
    }
    current_level = passage_level::verse;
    current_chapter = numbers.at(0);
  }
  else if(std::string_view("#-#X#") == section && numbers_size == 3)
  {
    if(current_level == passage_level::verse)
    {
      const auto ref1 = reference::create(book, current_chapter, numbers.at(0), versification);
      const auto ref2 = reference::create(book, numbers.at(1), numbers.at(2), versification);
      if(ref1 && ref2)
      {
        result.emplace_back(ref1.value(), ref2.value());
      }
    }
    else
    {
      const auto ref1 = reference::create(book, numbers.at(0), 1u, versification);
      const auto ref2 = reference::create(book, numbers.at(1), numbers.at(2), versification);
      if(ref1 && ref2)
      {
        result.emplace_back(ref1.value(), ref2.value());
      }
      current_level = passage_level::verse;
    }
    current_chapter = numbers.at(1);
  }
  else if(std::string_view("#X#") == section && numbers_size == 2)
  {
    const auto ref = reference::create(book, numbers.at(0), numbers.at(1), versification);
    if(ref)
    {
      result.emplace_back(ref.value());
    }
    current_level = passage_level::verse;
    current_chapter = numbers.at(0);
  }
  else if(std::string_view("#-#") == section && numbers_size == 2)
  {
    if(current_level == passage_level::verse)
    {
      const auto ref1 = reference::create(book, current_chapter, numbers.at(0), versification);
      const auto ref2 = reference::create(book, current_chapter, numbers.at(1), versification);
      if(ref1 && ref2)
      {
        result.emplace_back(ref1.value(), ref2.value());
      }
    }
    else
    {
      const auto verse_count = versification.verse_count(book, reference::chapter_type{numbers.at(1)});
      const auto ref1 = reference::create(book, numbers.at(0), 1u, versification);
      const auto ref2 = reference::create(book, numbers.at(1), verse_count, versification);
      if(ref1 && ref2)
      {
        result.emplace_back(ref1.value(), ref2.value());
      }
      current_chapter = numbers.at(1);
    }
  }
  else if(std::string_view("#") == section && numbers_size == 1)
  {
    if(current_level == passage_level::verse)
    {
      const auto ref = reference::create(book, current_chapter, numbers.front(), versification);
      if(ref)
      {
        result.emplace_back(ref.value());
      }
    }
    else
    {
      const auto verse_count = versification.verse_count(book, reference::chapter_type{numbers.front()});
      const auto ref1 = reference::create(book, numbers.front(), 1u, versification);
      const auto ref2 = reference::create(book, numbers.front(), verse_count, versification);
      if(ref1 && ref2)
      {
        result.emplace_back(ref1.value(), ref2.value());
      }
      current_chapter = numbers.front();
    }
  }
  return result;
}

} // namespace

///
///
auto reference_parser::parse(
  const std::string_view text, const std::size_t index, const util::language language, const versification& versification
) -> parse_result
{
  if(text.empty() || index >= text.size())
  {
    return parse_result{};
  }
  auto book = find_book(text, index, language);
  if(!book)
  {
    return parse_result{};
  }
  const auto passage_template =
    create_passage_template(text.substr(book->index_range_numbers.begin, math::size(book->index_range_numbers)), language);
  const auto index_range_origin = reference_index_range(text, index, *book, passage_template, language);
  if(!index_range_origin)
  {
    return parse_result{};
  }
  return parse_result{
    .ranges = match_passage_template(book->book, passage_template.passage_template, versification),
    .index_range_origin = *index_range_origin,
  };
}

///
///
auto reference_parser::find_book(const std::string_view text, const std::size_t index, const util::language language)
  -> std::optional<find_book_result>
{
  auto found_book = std::optional<find_book_result>{};

  std::string normalized_text;
  normalized_text.reserve(text.size());
  std::vector<index_range_type> raw_index_ranges;
  raw_index_ranges.reserve(text.size());
  txt::script_letters::visit(
    language,
    [&](const auto& letters)
    {
      txt::script_common::for_each_char(
        letters,
        text,
        [&](const auto character, const auto pos, const txt::script_common::category category) -> void
        {
          const auto append = [&](const std::string_view c) -> void
          {
            const auto size = c.size();
            if(size == 0)
            {
              return;
            }
            normalized_text.append(c.data(), size);
            const auto index_range = index_range_type{pos, pos + size};
            raw_index_ranges.insert(raw_index_ranges.end(), size, index_range);
          };
          switch(category)
          {
          case txt::script_common::category::letter: append(character); break;
          case txt::script_common::category::whitespace: /*noop*/ break;
          case txt::script_common::category::line: /*noop*/ break;
          case txt::script_common::category::fullstop: /*noop*/ break;
          case txt::script_common::category::digit: append(character); break;
          default: append("*"); break;
          }
        }
      );
    }
  );
  assert(raw_index_ranges.size() == normalized_text.size());

  // Reverse loop through all the book name variants because of two reasons:
  // 1. The common searches match more with the latter book names.
  // 2. For John and X_John the first match would be taken even if it should be the second one.
  std::ranges::for_each(
    ocr_book_variants::name_variants_with_aliases(language) | std::views::reverse |
      std::views::take_while([&]([[maybe_unused]] auto&) { return !found_book.has_value(); }),
    [&](const auto& element)
    {
      auto normalized_text_view = std::string_view{normalized_text};
      const auto& [book_id, name_variant] = element;

      auto pos_rel = std::size_t{0};
      auto pos_offset = std::size_t{0};
      std::ranges::for_each(
        util::ranges::index_view(normalized_text_view) |
          std::views::take_while([&]([[maybe_unused]] const auto /*i*/)
                                 { return pos_rel != std::string_view::npos && !found_book.has_value(); }),
        [&]([[maybe_unused]] const auto)
        {
          pos_rel = normalized_text_view.find(name_variant);
          if(pos_rel == std::string_view::npos)
          {
            return;
          }
          const auto pos_name_end = pos_rel + name_variant.size();
          const auto text_after_pos = normalized_text_view.substr(pos_name_end);
          if(const auto numbers_end_opt = find_numbers_after_book_name(text_after_pos, language))
          {
            const auto number_end = try_validate_numbers_range(text_after_pos, numbers_end_opt.value(), language);
            const auto pos_abs = pos_offset + pos_rel;

            const auto index_book_begin = raw_index_ranges.at(pos_abs).begin;
            const auto index_book_end = raw_index_ranges.at(pos_abs + name_variant.size() - 1).end;
            const auto index_numbers_begin = raw_index_ranges.at(pos_abs + name_variant.size()).begin;
            const auto index_numbers_end = raw_index_ranges.at(pos_abs + name_variant.size() + number_end - 1).end;

            if(math::contains(index_range_type{index_book_begin, index_numbers_end}, index))
            {
              found_book = find_book_result{
                .book = book_id,
                .index_range_book = index_range_type{   index_book_begin,    index_book_end},
                .index_range_numbers = index_range_type{index_numbers_begin, index_numbers_end},
                .book_name_variant = name_variant
              };
            }
          }
          pos_offset += pos_name_end;
          normalized_text_view = normalized_text_view.substr(pos_name_end);
        }
      );
    }
  );
  return found_book;
}

///
///
auto reference_parser::reference_index_range(
  const std::string_view text,
  const std::size_t index,
  const find_book_result& book,
  const passage_template_result& passage_template,
  const util::language language
) -> std::optional<index_range_type>
{
  // Without a passage number the reference ends with the book name.
  const auto end = passage_template.index_numbers_end > 0 ? book.index_range_numbers.begin + passage_template.index_numbers_end
                                                          : book.index_range_book.end;
  // The numbers found after the book name reach up to the next letter, the passage may end before. A number behind
  // it, e.g. the verse number "19" of "(Phil 1,10) 19 ...", is no part of the reference, the ")" of its word still is.
  if(index >= end_of_word(text, end, language))
  {
    return std::nullopt;
  }
  return index_range_type{book.index_range_book.begin, end};
}

///
///
auto reference_parser::end_of_word(const std::string_view text, const std::size_t index, const util::language language)
  -> std::size_t
{
  auto end = text.size();
  txt::script_letters::visit(
    language,
    [&](const auto& letters)
    {
      // finds the index of the first whitespace after the character at \p index
      txt::script_common::for_each_char_while(
        letters,
        text.substr(std::min(index, text.size())),
        [&]([[maybe_unused]] const auto character, const auto pos, const txt::script_common::category category)
        {
          const auto whitespace = category == txt::script_common::category::whitespace;
          if(whitespace)
          {
            end = index + pos;
          }
          return !whitespace;
        }
      );
    }
  );
  return end;
}

///
///
auto reference_parser::find_numbers_after_book_name(const std::string_view text_after_name, const util::language language)
  -> std::optional<std::size_t>
{
  auto digit_found = false;
  auto numbers_end = std::optional<std::size_t>{};
  txt::script_letters::visit(
    language,
    [&](const auto& letters)
    {
      txt::script_common::for_each_char_while(
        letters,
        text_after_name,
        [&](const auto character, const auto pos, const txt::script_common::category category)
        {
          if(category == txt::script_common::category::digit)
          {
            digit_found = true;
          }
          else if(
            category == txt::script_common::category::letter &&
            !util::contains(number_postfix_chars, [&](const auto v) { return util::string::starts_with(character, v); })
          )
          {
            numbers_end = pos;
          }
          return !numbers_end.has_value();
        }
      );
    }
  );
  return digit_found ? numbers_end.value_or(text_after_name.size()) : std::optional<std::size_t>{};
}

///
///
auto reference_parser::try_validate_numbers_range(
  const std::string_view text_after_name, std::size_t numbers_end, const util::language language
) -> std::size_t
{
  return txt::script_letters::visit(
    language,
    [&](const auto& letters) -> std::size_t
    {
      if(
        numbers_end < text_after_name.size() && numbers_end > 0 &&
        txt::script_common::is_char(letters, text_after_name, numbers_end, txt::script_common::category::letter)
      )
      {
        const auto text_from_last_number = text_after_name.substr(numbers_end - 1);
        const auto belongs_to_book_name = std::ranges::any_of(
          ocr_book_variants::name_variants_with_aliases(language),
          [&](const auto& element)
          {
            const auto& [_, name_variant] = element;
            return util::string::starts_with(text_from_last_number, name_variant);
          }
        );
        if(belongs_to_book_name)
        {
          --numbers_end;
        }
      }
      return numbers_end;
    }
  );
}

///
///
auto reference_parser::create_passage_template(const std::string_view passage_text, const util::language language)
  -> passage_template_result
{
  const auto normalized = normalize_passage_text(passage_text, language);
  const auto passage_substring = std::string_view{normalized.text};
  passage_template_type passage_template;
  auto index_numbers_end = std::size_t{0};
  auto pos = std::size_t{0};

  std::ranges::for_each(
    util::ranges::index_view(normalized.text) |
      std::views::take_while([&]([[maybe_unused]] auto) { return pos < normalized.text.size(); }),
    [&]([[maybe_unused]] auto)
    {
      if(!skip_gap(passage_substring, pos, passage_template))
      {
        pos = std::string_view::npos;
        return;
      }
      if(const auto number = identify_number(passage_substring, pos); number)
      {
        passage_template.emplace_back(number.value());
        index_numbers_end = normalized.raw_index_ranges.at(pos - 1).end;
      }
      // The number may be spaced out from its transition char, so the gap has to be checked again.
      if(!skip_gap(passage_substring, pos, passage_template))
      {
        pos = std::string_view::npos;
        return;
      }
      if(const auto transition_char = identify_transition(passage_substring, pos); transition_char)
      {
        if(!passage_template.empty() && std::holds_alternative<std::uint32_t>(passage_template.back()))
        {
          // Take first found transition chars and ignore further chars.
          passage_template.emplace_back(transition_char.value());
        }
      }
      else // Exit when no transition char is found.
      {
        pos = std::string_view::npos;
      }
    }
  );
  const auto is_number = [](const auto& e) { return std::holds_alternative<std::uint32_t>(e); };
  const auto first = std::ranges::find_if(passage_template, is_number);
  if(first != std::ranges::cend(passage_template))
  {
    passage_template.erase(std::ranges::cbegin(passage_template), first);
  }
  const auto [last, end_last] = std::ranges::find_last_if(passage_template, is_number);
  if(last != end_last)
  {
    passage_template.erase(std::next(last), std::ranges::cend(passage_template));
  }
  return passage_template_result{.passage_template = std::move(passage_template), .index_numbers_end = index_numbers_end};
}

///
///
auto reference_parser::normalize_passage_text(const std::string_view text, const util::language language) -> normalized_passage
{
  return txt::script_letters::visit(
    language,
    [&](const auto& letters) -> normalized_passage
    {
      normalized_passage result;
      auto counter = std::size_t{0};
      const auto append = [&](const std::string_view normalized, const std::size_t raw_size) -> void
      {
        result.text.append(normalized.data(), normalized.size());
        const auto index_range = index_range_type{counter, counter + raw_size};
        result.raw_index_ranges.insert(std::cend(result.raw_index_ranges), normalized.size(), index_range);
      };
      std::ranges::for_each(
        util::ranges::index_view(text) | std::views::take_while([&](const auto /*i*/) { return counter < text.size(); }),
        [&]([[maybe_unused]] const auto)
        {
          const auto subview = text.substr(counter);
          if(
            const auto iter = std::ranges::find_if(
              number_postfixes, [&](const auto postfix) { return util::string::starts_with(subview, postfix); }
            );
            iter != std::ranges::cend(number_postfixes)
          )
          {
            append(std::string_view{&gap, 1}, iter->size()); // Ignore possible postfixes
            counter += iter->size();
          }
          else if(const auto data = txt::script_common::char_info(letters, subview, 0); data)
          {
            switch(data->char_category)
            {
            case txt::script_common::category::letter: [[fallthrough]];
            case txt::script_common::category::whitespace: append(std::string_view{&gap, 1}, data->char_size); break;
            case txt::script_common::category::line: append("-", data->char_size); break;
            default: append(subview.substr(0, data->char_size), data->char_size); break;
            }
            counter += data->char_size;
          }
          else
          {
            LOG_ERROR("invalid char info: char=\'{}\'", subview.at(0));
            ++counter;
          }
        }
      );
      assert(result.raw_index_ranges.size() == result.text.size());
      return result;
    }
  );
}

///
///
auto reference_parser::identify_number(std::string_view text, std::size_t& pos) -> std::optional<std::uint32_t>
{
  const auto number = txt::find_uint(text.substr(pos));
  if(number)
  {
    pos += number->post_value_offset;
    return number->value;
  }
  return std::nullopt;
}

///
///
auto reference_parser::identify_transition(const std::string_view text, std::size_t& pos) -> std::optional<char>
{
  if(pos >= text.size())
  {
    return std::nullopt;
  }
  const auto transition_char = text.at(pos);
  if(util::contains(transition_chars, transition_char))
  {
    ++pos;
    return transition_char;
  }
  return std::nullopt;
}

///
///
auto reference_parser::skip_gap(
  const std::string_view text, std::size_t& pos, const passage_template_type& current_passage_template
) -> bool
{
  if(pos >= text.size() || text.at(pos) != gap)
  {
    return true;
  }
  auto end = pos;

  const auto gap_size =
    std::ranges::count(text.substr(pos) | std::views::take_while([](const auto c) { return c == gap; }), gap);
  end += gap_size;

  const auto transition_pending = [&]() -> std::optional<char>
  {
    const auto holds_char = !current_passage_template.empty() && std::holds_alternative<char>(current_passage_template.back());
    return holds_char ? std::optional{std::get<char>(current_passage_template.back())} : std::nullopt;
  }();
  // A pending full stop ends a sentence instead of waiting for a number, so it does not bridge the gap.
  const auto transition_bridges = transition_pending && *transition_pending != fullstop_char;
  const auto transition_follows = end < text.size() && util::contains(transition_chars, text.at(end));
  const auto result = transition_bridges || transition_follows;
  if(result)
  {
    pos = end;
  }
  return result;
}

///
///
auto reference_parser::match_passage_template(
  const book_id book, const passage_template_type& passage_template, const versification& versification
) -> std::vector<reference_range>
{
  if(!util::valid(book))
  {
    throw util::exception("invalid book ID");
  }
  auto result = std::vector<reference_range>{};
  const auto down_transition_chars = passage_template_transition_chars(passage_template);
  const auto numbers = passage_template_numbers(passage_template);
  if(passage_template.empty())
  {
    // find_book only reports a numbers range when it contains a digit, empty
    // template means none of those digits could be read as a passage number.
    return result;
  }
  else if(down_transition_chars.empty())
  {
    // This should result to only one passage section either # or #-#.
    const auto passage_sections = create_passage_sections(passage_template, std::nullopt);
    if(passage_sections.empty())
    {
      return result;
    }
    else if(passage_sections.size() > 1)
    {
      LOG_ERROR("unexpected passage section detected: count={}, expected=1", passage_sections.size());
      return result;
    }
    auto current_level = passage_level::chapter;
    auto current_chapter = numbers.front();
    const auto found = match_passage_template_section(
      book,
      passage_sections.front().numbers,
      passage_sections.front().generic_template,
      current_level,
      current_chapter,
      versification
    );
    result.insert(result.cend(), found.cbegin(), found.cend());
    return result;
  }

  std::map<char, std::vector<reference_range>> reference_ranges;
  for(const auto down_transition_char : down_transition_chars)
  {
    const auto passage_sections = create_passage_sections(passage_template, down_transition_char);
    auto current_level = passage_level::chapter;
    auto current_chapter = numbers.front();
    std::ignore = std::ranges::all_of(
      passage_sections,
      [&](const auto& passage_section)
      {
        const auto found = match_passage_template_section(
          book, passage_section.numbers, passage_section.generic_template, current_level, current_chapter, versification
        );
        const auto result = !found.empty();
        if(result)
        {
          decltype(auto) ranges = reference_ranges[down_transition_char];
          ranges.insert(ranges.cend(), found.cbegin(), found.cend());
        }
        else
        {
          reference_ranges.erase(down_transition_char);
        }
        return result;
      }
    );
  }

  auto reference_ranges_view = reference_ranges | std::views::filter([](const auto& p) { return !p.second.empty(); });
  std::vector<std::pair<char, std::uint32_t>> reference_ranges_verse_count;
  std::ranges::for_each(
    reference_ranges_view,
    [&](const auto& pair)
    {
      const auto& [c, references] = pair;
      const auto verse_count = std::ranges::fold_left(
        references,
        std::uint32_t{0},
        [&](const std::uint32_t total, const auto& ref) { return total + versification.size(ref).value(); }
      );
      reference_ranges_verse_count.emplace_back(std::pair{c, verse_count});
    }
  );
  const auto reference_ranges_iter =
    std::ranges::min_element(reference_ranges_verse_count, [](const auto& a, const auto& b) { return a.second < b.second; });
  if(reference_ranges_iter != std::ranges::cend(reference_ranges_verse_count))
  {
    result = std::move(reference_ranges.at(reference_ranges_iter->first));
  }
  return result;
}

///
///
auto reference_parser::passage_template_transition_chars(const passage_template_type& passage_template) -> std::vector<char>
{
  auto result = std::vector<char>{};
  std::ranges::for_each(
    passage_template | std::views::filter([](const auto e) { return std::holds_alternative<char>(e); }) |
      std::views::transform([](const auto e) { return std::get<char>(e); }) |
      std::views::filter([](const auto c) { return c != '-'; }) |
      std::views::filter([&](const auto c) { return !util::contains(result, c); }),
    [&](const auto c) { result.push_back(c); }
  );
  return result;
}

///
///
auto reference_parser::passage_template_numbers(const passage_template_type& passage_template) -> std::vector<std::uint32_t>
{
  auto result = std::vector<std::uint32_t>{};
  std::ranges::for_each(
    passage_template | std::views::filter([](const auto e) { return std::holds_alternative<std::uint32_t>(e); }) |
      std::views::transform([](const auto e) { return std::get<std::uint32_t>(e); }),
    [&](const auto n) { result.push_back(n); }
  );
  return result;
}

///
///
auto reference_parser::create_passage_sections(
  const passage_template_type& passage_template, const std::optional<char> down_transition_char
) -> std::vector<passage_section>
{
  const auto to_transition_char = [down_transition_char](const char c) -> std::optional<char>
  {
    if(c == '-')
    {
      return '-';
    }
    else if(c == down_transition_char)
    {
      return 'X';
    }
    else
    {
      return std::nullopt;
    }
  };

  std::vector<passage_section> result;
  passage_section current_result;
  const auto handle_uint32_t = [&](const std::uint32_t n)
  {
    current_result.numbers.push_back(n);
    current_result.generic_template.push_back('#');
  };
  const auto handle_char = [&](const char c)
  {
    if(const auto transition_char = to_transition_char(c); transition_char)
    {
      current_result.generic_template.push_back(*transition_char);
    }
    else
    {
      result.emplace_back(std::move(current_result));
      current_result = passage_section{};
    }
  };
  std::ranges::for_each(passage_template, [&](const auto e) { util::visit_lambdas(e, handle_uint32_t, handle_char); });
  result.emplace_back(std::move(current_result));
  return result;
}

} // namespace bibstd::bible
