#include "bibstd/bible/reference_ocr.hpp"
#include "bibstd/data/pixel.hpp"
#include "bibstd/data/screen_types.hpp"
#include "bibstd/math/coordinates.hpp"
#include "bibstd/math/rect.hpp"
#include "bibstd/txt/ocr_engine.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/numeric_cast.hpp"
#include "bibstd/util/ranges.hpp"
#include "bibstd/util/timer.hpp"
#include "bibstd/util/visit_helper.hpp"

#include <boost/gil/extension/numeric/resample.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/image_view_factory.hpp>
#include <boost/gil/typedefs.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace bibstd::bible
{
namespace
{

///
/// Concept to check if a type has a member named paragraph_data.
///
template<typename T>
concept has_paragraph_data = requires(T t) { t.paragraph_data; };

///
/// Position of one character: the word that supplied it and its bounding box.
///
using character_position_type = std::optional<std::pair<std::size_t, typename txt::ocr_engine<>::bounding_box_type>>;

///
/// Consecutive words of the recognition data, given as the index range [begin, end) into it. The
/// engines report their words in reading order, so the words of one line or paragraph are consecutive.
///
struct word_range final
{
  // Variables
  std::size_t begin{0};
  std::size_t end{0};

  ///
  /// \return true if the word belongs to the range
  ///
  [[nodiscard]] auto contains(const std::size_t word_index) const -> bool { return word_index >= begin && word_index < end; }

  ///
  /// \return indices of the words in reading order
  ///
  [[nodiscard]] auto indices() const -> auto { return util::ranges::index_view_between(begin, end); }
};

///
/// This struct holds the text a reference is searched in, together
/// with the words it was built from.
///
struct position_text final
{
  ///
  /// Text one recognized line or paragraph supplied, together with the words written in it.
  ///
  struct text_run final
  {
    // Position of the run within the text.
    std::size_t begin;
    std::size_t size;
    // Words the run was written from.
    word_range words;
  };

  // Variables
  std::string text;
  std::vector<text_run> runs;
  // Words of the line that was pointed at.
  word_range focus_line_words;
};

///
/// Consecutive characters which all have a bounding box, e.g. a word.
///
struct character_run final
{
  math::value_range<std::size_t> index_range;
  data::screen_rect_type bounding_box;
};

///
/// One recognized line together with the words written on it.
///
template<typename LineType>
struct line_words final
{
  std::reference_wrapper<const LineType> line;
  word_range words;
};

///
/// Access the name of the OCR engine.
/// \return engine name
///
[[nodiscard]] auto name(const txt::ocr_engine_uptr_variant_type& engine) -> std::string
{
  return util::visit_lambdas(
    engine,
    []([[maybe_unused]] const std::monostate&) { return std::string{"Undefined"}; },
    [](const auto& e) { return std::string{e->name()}; }
  );
}

///
/// Widen the range around the given word as long as the predicate accepts the neighbour. A word the
/// predicate rejects ends the range, so words it accepts elsewhere in the data stay out of it.
/// \return consecutive accepted words around the given one
///
[[nodiscard]] auto words_around(const auto& recognition_data, const std::size_t word_index, const auto& predicate) -> word_range
{
  const auto accepted = [&](const std::size_t index) { return predicate(recognition_data.at(index)); };
  const auto accepted_count = [&](auto indices)
  { return static_cast<std::size_t>(std::ranges::distance(indices | std::views::take_while(accepted))); };
  return word_range{
    .begin = word_index - accepted_count(util::ranges::index_view_to(word_index) | std::views::reverse),
    .end = word_index + 1 + accepted_count(util::ranges::index_view_between(word_index + 1, recognition_data.size()))
  };
}

///
/// Take the element at \p it together with the element before and after it, as far as they exist.
/// \return subrange of \p range around \p it
///
[[nodiscard]] auto with_neighbours(const auto& range, const auto it) -> auto
{
  const auto first = it == std::ranges::cbegin(range) ? it : std::ranges::prev(it);
  const auto end = std::ranges::next(it) == std::ranges::cend(range) ? std::ranges::cend(range) : std::ranges::next(it, 2);
  return std::ranges::subrange{first, end};
}

///
/// Check whether the recognized element sits on the line with the given bounding box. Every word of
/// a line carries the bounding box of its own line, so equality identifies the line.
/// \return true if the element sits on that line
///
[[nodiscard]] auto on_line(const auto& element, const auto& line_bounding_box) -> bool
{
  return element.line_data && element.line_data->bounding_box == line_bounding_box;
}

///
/// Take the text of one recognized element as a whole, written by the given words. The engines break the text of a
/// paragraph into lines, a line break separates words just like a space does.
/// \return text with a space for every line break and a single run over all of it
///
[[nodiscard]] auto whole_of(const std::string_view element_text, const word_range words) -> position_text
{
  return position_text{
    .text = element_text | std::views::transform([](const char c) { return c == '\n' || c == '\r' ? ' ' : c; }) |
            std::ranges::to<std::string>(),
    .runs = {position_text::text_run{.begin = 0, .size = element_text.size(), .words = words}},
    .focus_line_words = {}
  };
}

///
/// Take the text of the paragraph the cursor word sits in. Only an engine with layout analysis
/// reports paragraphs at all.
/// \return text of the paragraph and the words written in it, nothing without a paragraph
///
[[nodiscard]] auto paragraph_text(const auto& recognition_data, const std::size_t cursor_word_index)
  -> std::optional<position_text>
{
  const auto& cursor_element = recognition_data.at(cursor_word_index);
  if constexpr(has_paragraph_data<decltype(cursor_element)>)
  {
    if(cursor_element.paragraph_data)
    {
      const auto& paragraph_bounding_box = cursor_element.paragraph_data->bounding_box;
      const auto in_paragraph = [&](const auto& e)
      { return e.paragraph_data && e.paragraph_data->bounding_box == paragraph_bounding_box; };
      return whole_of(cursor_element.paragraph_data->text, words_around(recognition_data, cursor_word_index, in_paragraph));
    }
  }
  return std::nullopt;
}

///
/// Group the recognized words into the lines they sit on. Every word carries the line it sits on and
/// the words are reported in reading order, so a change of the line ends the current group.
/// \return lines in reading order, each with the words written on it
///
[[nodiscard]] auto collect_lines(const auto& recognition_data) -> auto
{
  using element_type = std::ranges::range_value_t<std::remove_cvref_t<decltype(recognition_data)>>;
  using line_type = typename decltype(element_type::line_data)::value_type;

  auto result = std::vector<line_words<line_type>>{};
  std::ranges::for_each(
    recognition_data | std::views::enumerate,
    [&](const auto& p)
    {
      const auto& [index, element] = p;
      if(!element.line_data)
      {
        return;
      }
      const auto word_index = static_cast<std::size_t>(index);
      const auto words = word_range{.begin = word_index, .end = word_index + 1};
      if(result.empty() || result.back().line.get().bounding_box != element.line_data->bounding_box)
      {
        result.emplace_back(*element.line_data, words);
      }
      else
      {
        result.back().words.end = words.end;
      }
    }
  );
  return result;
}

///
/// Join the text of the given lines, each of them followed by a space.
/// \return text of the lines with one run per line
///
[[nodiscard]] auto text_of_lines(const auto& lines) -> position_text
{
  auto result = position_text{};
  std::ranges::for_each(
    lines,
    [&](const auto& l)
    {
      // Some engines end their line text with a line break, the space alone separates the lines.
      const auto line_text = std::string_view{l.line.get().text};
      const auto begin = result.text.size();
      result.text.append(line_text.substr(0, line_text.find_last_not_of("\r\n") + 1));
      result.text.push_back(' ');
      result.runs.emplace_back(position_text::text_run{.begin = begin, .size = result.text.size() - begin, .words = l.words});
    }
  );
  return result;
}

///
/// Collect the text belonging to the recognized element the position falls into. An engine with layout
/// analysis groups the lines into paragraphs itself, an engine without reports lines only. There the line
/// above and below are taken as well, otherwise a reference broken over a line break is cut in half.
/// \return text around the position, the words it was taken from and the words of the pointed at line
///
[[nodiscard]] auto text_around(const auto& recognition_data, const std::size_t cursor_word_index) -> position_text
{
  const auto& cursor_element = recognition_data.at(cursor_word_index);

  // Words of the line the cursor sits on. Without line data only the word pointed at is left.
  const auto focus_line_words =
    cursor_element.line_data
      ? words_around(
          recognition_data, cursor_word_index, [&](const auto& e) { return on_line(e, cursor_element.line_data->bounding_box); }
        )
      : word_range{.begin = cursor_word_index, .end = cursor_word_index + 1};
  const auto with_focus_line = [&](position_text text)
  {
    text.focus_line_words = focus_line_words;
    return text;
  };

  if(auto paragraph = paragraph_text(recognition_data, cursor_word_index))
  {
    return with_focus_line(std::move(*paragraph));
  }
  if(!cursor_element.line_data)
  {
    return with_focus_line(whole_of(cursor_element.word_data.text, focus_line_words));
  }

  const auto lines = collect_lines(recognition_data);
  const auto cursor_line = std::ranges::find_if(lines, [&](const auto& l) { return l.words.contains(cursor_word_index); });
  if(cursor_line == std::ranges::cend(lines))
  {
    LOG_WARN("cursor line not found in lines: cursor line bounding box: {}", cursor_element.line_data->bounding_box);
    return with_focus_line(whole_of(cursor_element.line_data->text, focus_line_words));
  }

  return with_focus_line(text_of_lines(with_neighbours(lines, cursor_line)));
}

///
/// Approximate the bounding box of every character of the word by dividing the bounding box of the
/// word into equally wide parts.
/// \return one bounding box per character of the word in reading order
///
[[nodiscard]] auto character_boxes_of(const auto& word) -> std::vector<std::remove_cvref_t<decltype(word.bounding_box)>>
{
  const auto word_h_range = math::size(word.bounding_box.horizontal_range());
  const auto char_width_approx = word_h_range / numeric_cast<decltype(word_h_range)>(word.text.size());
  const auto char_width_approx_signed = numeric_cast<std::make_signed_t<decltype(char_width_approx)>>(char_width_approx);
  const auto word_origin = word.bounding_box.origin();

  return util::ranges::index_view_to(word.text.size()) |
         std::views::transform(
           [&](const auto i)
           {
             const auto shifted_x = word_origin.x() + (i * char_width_approx_signed);
             return std::remove_cvref_t<decltype(word.bounding_box)>{
               decltype(word_origin){shifted_x, word_origin.y()},
               char_width_approx,
               math::size(word.bounding_box.vertical_range())
             };
           }
         ) |
         std::ranges::to<std::vector>();
}

///
/// Generate character positions by matching every word of a text run to the text the run holds. A word
/// is only looked for in the run it supplied its text to. A word of a neighboring line would otherwise
/// match a word of the same spelling anywhere in the text and drag the search past everything that
/// follows it.
/// \return list of character positions, one for each char of the text
///
[[nodiscard]] auto generate_character_positions(const auto& recognition_data, const position_text& position)
  -> std::vector<character_position_type>
{
  auto result = std::vector<character_position_type>(position.text.size());

  std::ranges::for_each(
    position.runs,
    [&](const auto& run)
    {
      const auto run_text = std::string_view{position.text}.substr(run.begin, run.size);

      // The words are searched in reading order and each one continues behind its predecessor, so a
      // word repeated within the run takes the occurrence belonging to it.
      auto search_begin = std::size_t{0};
      std::ranges::for_each(
        run.words.indices(),
        [&](const auto word_index)
        {
          const auto& word = recognition_data.at(word_index).word_data;
          // A word without text supplies no character and cannot be located.
          if(word.text.empty())
          {
            return;
          }
          const auto pos = run_text.find(std::string_view{word.text}, search_begin);
          if(pos == std::string_view::npos)
          {
            LOG_WARN("expected word not found: \"{}\"", word.text);
            return;
          }
          search_begin = pos + word.text.size();

          const auto boxes = character_boxes_of(word);
          const auto word_begin = run.begin + pos;
          std::ranges::for_each(
            util::ranges::index_view(boxes), [&](const auto i) { result.at(word_begin + i) = {word_index, boxes.at(i)}; }
          );
        }
      );
    }
  );
  return result;
}

///
/// Find the word the position points at. A position between two words falls into no word, there the
/// line it points at is taken.
/// \return word pointed at as index into the recognition data, nothing if the position points nowhere
///
[[nodiscard]] auto find_cursor_word(const auto& recognition_data, const reference_ocr::position_type position)
  -> std::optional<std::size_t>
{
  const auto index_of = [&](const auto& it)
  { return static_cast<std::size_t>(std::ranges::distance(std::ranges::cbegin(recognition_data), it)); };
  const auto in_word = [&](const auto& d) { return math::contains(d.word_data.bounding_box, position); };
  const auto in_line = [&](const auto& d) { return d.line_data && math::contains(d.line_data->bounding_box, position); };

  if(const auto it = std::ranges::find_if(recognition_data, in_word); it != std::ranges::cend(recognition_data))
  {
    return index_of(it);
  }
  if(const auto it = std::ranges::find_if(recognition_data, in_line); it != std::ranges::cend(recognition_data))
  {
    return index_of(it);
  }
  return std::nullopt;
}

///
/// Search the character closest to the position among those the words of the focus line supplied.
/// Only a character of the line that was pointed at can be the one under the cursor.
/// \return index of the character within the text, nothing if no such character was located
///
[[nodiscard]] auto find_closest_character(
  const std::vector<character_position_type>& character_positions,
  const word_range focus_line_words,
  const reference_ocr::position_type position
) -> std::optional<std::size_t>
{
  const auto to_distance = [&](const auto& p)
  {
    return p && focus_line_words.contains(p->first) ? reference_ocr::position_type::distance(p->second.center(), position)
                                                    : std::numeric_limits<double>::max();
  };
  const auto distance_view = character_positions | std::views::transform(to_distance);
  const auto it = std::ranges::min_element(distance_view, std::less{});
  // Without a single located character on that line there is no cursor index. Taking the closest
  // one anyway lands on the first character of the text, a reference that is never pointed at.
  if(it == std::ranges::cend(distance_view) || *it == std::numeric_limits<double>::max())
  {
    return std::nullopt;
  }
  return static_cast<std::size_t>(std::ranges::distance(std::ranges::cbegin(distance_view), it));
}

///
/// Find the character of the recognized text around the position that the position points at.
/// \return text around the position with the index of that character, empty position data if the position points nowhere
///
[[nodiscard]] auto find_index(const auto& recognition_data, const reference_ocr::position_type position)
  -> std::expected<reference_ocr::reference_position_data, reference_ocr::unexpected_ocr_result>
{
  if(recognition_data.empty())
  {
    LOG_DEBUG("returns empty: recognition_data is empty");
    return reference_ocr::reference_position_data{};
  }
  const auto cursor_word_index = find_cursor_word(recognition_data, position);
  if(!cursor_word_index)
  {
    LOG_DEBUG("returns empty: position is not contained in any word bounding box");
    return reference_ocr::reference_position_data{};
  }

  const auto position_data = text_around(recognition_data, *cursor_word_index);
  const auto& text = position_data.text;
  const auto character_positions = generate_character_positions(recognition_data, position_data);
  assert(character_positions.size() == text.size());

  const auto index = find_closest_character(character_positions, position_data.focus_line_words, position);
  if(!index)
  {
    LOG_DEBUG("returns empty: no character of the pointed at line was located in the recognized text");
    return reference_ocr::reference_position_data{};
  }
  LOG_DEBUG(
    "returns reference position data: text=\"{}[{}]{}\"", text.subview(0, *index), text.at(*index), text.subview(*index + 1)
  );
  auto boxes = character_positions |
               std::views::transform([](const auto& p) { return p ? std::make_optional(p->second) : std::nullopt; }) |
               std::ranges::to<std::vector>();
  return reference_ocr::reference_position_data{text, *index, std::move(boxes)};
}

///
/// Shift all character bounding boxes of the position data by the specified offset.
/// This is needed to convert bounding boxes that are relative to a recognition
/// subarea into the coordinate system of the whole image.
/// \return position data with shifted character bounding boxes
///
[[nodiscard]] auto shift_character_bounding_boxes(
  std::expected<reference_ocr::reference_position_data, reference_ocr::unexpected_ocr_result> position_data,
  const reference_ocr::position_type offset
) -> std::expected<reference_ocr::reference_position_data, reference_ocr::unexpected_ocr_result>
{
  if(position_data)
  {
    std::ranges::for_each(
      position_data->character_bounding_boxes | std::views::filter([](const auto& box) { return box.has_value(); }),
      [&](auto& box)
      {
        using box_type = typename std::remove_reference_t<decltype(box)>::value_type;
        box = box_type{box->origin() + offset, math::size(box->horizontal_range()), math::size(box->vertical_range())};
      }
    );
  }
  return position_data;
}

///
/// Get character recognition engine from engine list using the specified name.
/// \return a reference wrapper of the engine as expected result or an error code as unexpected result.
///
[[nodiscard]] auto get_character_recognition_engine(
  const reference_ocr::ocr_engine_list_type& engines, const reference_ocr::engine_names& names
) -> std::expected<std::reference_wrapper<const txt::ocr_engine_uptr_variant_type>, reference_ocr::unexpected_ocr_result>
{
  const auto character_recognition_engine_it =
    std::ranges::find_if(engines, [&](const auto& e) { return name(e) == names.character_recognition; });
  if(character_recognition_engine_it == std::ranges::cend(engines))
  {
    LOG_ERROR("ocr engine for character recognition not found: required=\"{}\"", names.character_recognition);
    return std::unexpected{reference_ocr::unexpected_ocr_result::error};
  }
  return std::ref(*character_recognition_engine_it);
}

///
/// Pad the area by half the height of \p line on every side. The padding keeps the characters off the edge of the
/// recognized area, engines read them better there.
/// \return padded area
///
[[nodiscard]] auto padded(const data::screen_rect_type& area, const data::screen_rect_type& line) -> data::screen_rect_type
{
  const auto padding = math::size(line.vertical_range()) / 2;
  const auto signed_padding = numeric_cast<data::screen_rect_type::value_type>(padding);
  return data::screen_rect_type{
    math::coordinates(area.origin().x() - signed_padding, area.origin().y() - signed_padding),
    math::size(area.horizontal_range()) + (2 * padding),
    math::size(area.vertical_range()) + (2 * padding)
  };
}

///
/// Take the area of the given line together with the line above and below it, as far as they belong
/// to the same paragraph. A reference broken over a line break is only found completely if the line
/// it continues on is recognized as well.
/// \return area of the relevant lines with a bit of padding around them
///
[[nodiscard]] auto relevant_lines_area(const auto& layouts, const auto& relevant_line_it) -> data::screen_rect_type
{
  const auto& relevant_line = *relevant_line_it;
  const auto same_paragraph_line = [&](const auto& it)
  {
    return it->paragraph_bounding_box == relevant_line.paragraph_bounding_box ? it->line_bounding_box
                                                                              : relevant_line.line_bounding_box;
  };
  const auto prev = relevant_line_it == std::ranges::cbegin(layouts) ? relevant_line.line_bounding_box
                                                                     : same_paragraph_line(std::ranges::prev(relevant_line_it));
  const auto next = std::ranges::next(relevant_line_it) == std::ranges::cend(layouts)
                      ? relevant_line.line_bounding_box
                      : same_paragraph_line(std::ranges::next(relevant_line_it));
  return padded(math::surrounding_rect(prev, relevant_line.line_bounding_box, next), relevant_line.line_bounding_box);
}

///
/// Run paragraph recognition on the image. This requires an engine that supports layout analysis.
/// If a paragraph is found the line corresponding to the position is taken and the area of this
/// line, the previous and the next line is returned to reduce the relevant area even more.
/// \return bounding box with the relevant lines or an error code if something unexpected happens
///
[[nodiscard]] auto run_paragraph_recognition(
  const reference_ocr::ocr_engine_list_type& engines,
  const reference_ocr::engine_names& names,
  const reference_ocr::pixel_plane_view_type& image,
  const reference_ocr::position_type position
) -> std::expected<data::screen_rect_type, reference_ocr::unexpected_ocr_result>
{
  SCOPED_TIMER_LOG();
  using return_type = std::expected<data::screen_rect_type, reference_ocr::unexpected_ocr_result>;

  if(!names.layout_recognition)
  {
    LOG_ERROR("ocr engine for paragraph recognition not specified");
    return std::unexpected{reference_ocr::unexpected_ocr_result::error};
  }
  const auto engine_it = std::ranges::find_if(engines, [&](const auto& e) { return name(e) == names.layout_recognition; });
  if(engine_it == std::ranges::cend(engines))
  {
    LOG_ERROR("ocr engine for paragraph recognition not found: required=\"{}\"", *names.layout_recognition);
    return std::unexpected{reference_ocr::unexpected_ocr_result::error};
  }
  const auto& engine_variant = *engine_it;
  return util::visit_lambdas(
    engine_variant,
    [&]([[maybe_unused]] const std::monostate&) -> return_type
    {
      LOG_ERROR("undefined ocr engine does not support paragraph recognition");
      return std::unexpected{reference_ocr::unexpected_ocr_result::error};
    },
    [&](const txt::ocr_engine<txt::ocr_engine_tag_plain>::uptr_type& e) -> return_type
    {
      LOG_ERROR("ocr engine \"{}\" does not support paragraph recognition", e->name());
      return std::unexpected{reference_ocr::unexpected_ocr_result::error};
    },
    [&](const txt::ocr_engine<txt::ocr_engine_tag_layout_analysis>::uptr_type& e) -> return_type
    {
      auto& engine = *e;
      engine.initialize(image, std::nullopt);
      const auto layouts = engine.layout_analysis();
      const auto relevant_line_it =
        std::ranges::find_if(layouts, [&](const auto& line) { return math::contains(line.line_bounding_box, position); });
      if(relevant_line_it == std::ranges::cend(layouts))
      {
        LOG_DEBUG("paragraph recognition returns with empty rect");
        return data::screen_rect_type{math::coordinates(0, 0), 0u, 0u};
      }
      if(!relevant_line_it->paragraph_bounding_box)
      {
        return relevant_line_it->line_bounding_box;
      }
      return relevant_lines_area(layouts, relevant_line_it);
    }
  );
}

///
/// Recognize text in image using layout analysis and recognition on the reduced area
/// \see run_paragraph_recognition.
/// \return text with character index corresponding to the specified position
///
[[nodiscard]] auto recognize_with_paragraph_recognition(
  const reference_ocr::ocr_engine_list_type& engines,
  const reference_ocr::engine_names& names,
  const reference_ocr::pixel_plane_view_type& image,
  const reference_ocr::position_type position
) -> std::expected<reference_ocr::reference_position_data, reference_ocr::unexpected_ocr_result>
{
  using return_type = std::expected<reference_ocr::reference_position_data, reference_ocr::unexpected_ocr_result>;
  const auto engine_ref = get_character_recognition_engine(engines, names);
  if(!engine_ref)
  {
    return std::unexpected{engine_ref.error()};
  }
  const auto area = run_paragraph_recognition(engines, names, image, position);
  if(!area)
  {
    return std::unexpected{area.error()};
  }

  // The padding added around the lines can reach outside the image. An engine recognizes the
  // area clipped to the image, so the character boxes it reports are relative to the clipped
  // area and it is that origin the boxes have to be shifted back by.
  const auto clipped = math::overlap(*area, data::screen_rect_type{math::coordinates(0, 0), image.width(), image.height()});
  if(!clipped || math::empty(*clipped))
  {
    // empty position data
    return reference_ocr::reference_position_data{};
  }

  const auto origin = clipped->origin();
  const auto relative_position = position - origin;
  return util::visit_lambdas(
    engine_ref->get(),
    []([[maybe_unused]] const std::monostate&) -> return_type
    { return std::unexpected{reference_ocr::unexpected_ocr_result::error}; },
    [&](const auto& e) -> return_type
    {
      SCOPED_TIMER_LOG();
      e->initialize(image, *clipped);
      return shift_character_bounding_boxes(find_index(e->recognize(), relative_position), origin);
    }
  );
}

///
/// Recognize the whole image with the character recognition engine, without a layout analysis.
/// \return text with character index corresponding to the specified position
///
[[nodiscard]] auto recognize_just_with_line_recognition(
  const reference_ocr::ocr_engine_list_type& engines,
  const reference_ocr::engine_names& names,
  const reference_ocr::pixel_plane_view_type& image,
  const reference_ocr::position_type position
) -> std::expected<reference_ocr::reference_position_data, reference_ocr::unexpected_ocr_result>
{
  using return_type = std::expected<reference_ocr::reference_position_data, reference_ocr::unexpected_ocr_result>;
  const auto engine_ref = get_character_recognition_engine(engines, names);
  if(!engine_ref)
  {
    return std::unexpected{engine_ref.error()};
  }
  return util::visit_lambdas(
    engine_ref->get(),
    []([[maybe_unused]] const std::monostate&) -> return_type
    { return std::unexpected{reference_ocr::unexpected_ocr_result::error}; },
    [&](const auto& e) -> return_type
    {
      e->initialize(image, std::nullopt);
      return find_index(e->recognize(), position);
    }
  );
}

///
/// Split the text into runs, a character without a bounding box, e.g. a space, separates two runs.
/// \return runs of the text in reading order, none without a box per character
///
[[nodiscard]] auto character_runs(const reference_ocr::reference_position_data& position_data) -> std::vector<character_run>
{
  const auto& boxes = position_data.character_bounding_boxes;
  if(boxes.size() != position_data.text.size())
  {
    return {};
  }
  const auto located = [&](const std::size_t index) { return boxes.at(index).has_value(); };
  const auto box_of = [&](const std::size_t index) { return *boxes.at(index); };
  const auto surrounding = [](const auto& a, const auto& b) { return math::surrounding_rect(a, b); };
  const auto to_run = [&](const auto indices)
  {
    return character_run{
      .index_range = math::value_range<std::size_t>{indices.front(), indices.back() + 1},
      .bounding_box = *std::ranges::fold_left_first(indices | std::views::transform(box_of), surrounding)
    };
  };
  return util::ranges::index_view_to(position_data.text.size()) |
         std::views::chunk_by([&](const auto a, const auto b) { return located(a) == located(b); }) |
         std::views::filter([&](const auto indices) { return located(indices.front()); }) | std::views::transform(to_run) |
         std::ranges::to<std::vector>();
}

///
/// The runs of a line follow each other from left to right at the same height.
/// \return true if \p after continues the line of \p before
///
[[nodiscard]] auto on_same_line(const character_run& before, const character_run& after) -> bool
{
  return before.bounding_box.origin().x() < after.bounding_box.origin().x() &&
         math::overlaps(before.bounding_box.vertical_range(), after.bounding_box.vertical_range());
}

///
/// Find the gaps bordering a run of \p index_range. A character the engine dropped leaves its width in the gap, so
/// only a gap up to the typical gap plus the typical character width of the line keeps the characters consecutive.
/// \return runs on both sides of every wider gap
///
[[nodiscard]] auto wide_gaps_in_line(
  const std::span<const character_run> runs, const math::value_range<std::size_t> index_range
) -> std::vector<std::pair<character_run, character_run>>
{
  // The lower median, of two gaps the wide gap of dropped text would otherwise be the typical gap itself.
  static constexpr auto median_of = [](std::vector<double> values)
  {
    std::ranges::sort(values);
    return values.at((values.size() - 1) / 2);
  };
  const auto left_of = [](const character_run& run) { return static_cast<double>(run.bounding_box.origin().x()); };
  const auto width_of = [](const character_run& run)
  { return static_cast<double>(math::size(run.bounding_box.horizontal_range())); };

  // With fewer runs there is no typical gap to compare with.
  if(runs.size() < 3)
  {
    return {};
  }
  const auto gaps =
    runs | std::views::pairwise_transform([&](const auto& a, const auto& b) { return left_of(b) - left_of(a) - width_of(a); }) |
    std::ranges::to<std::vector>();
  const auto character_widths =
    runs |
    std::views::transform([&](const auto& run) { return width_of(run) / static_cast<double>(math::size(run.index_range)); }) |
    std::ranges::to<std::vector>();
  const auto threshold = median_of(gaps) + median_of(character_widths);
  const auto in_index_range = [&](const character_run& run) { return math::overlaps(run.index_range, index_range); };
  return std::views::zip(gaps, runs, runs | std::views::drop(1)) |
         std::views::filter(
           [&](const auto& gap_between)
           {
             const auto& [gap, before, after] = gap_between;
             return gap > threshold && (in_index_range(before) || in_index_range(after));
           }
         ) |
         std::views::transform(
           [](const auto& gap_between)
           {
             const auto& [_, before, after] = gap_between;
             return std::pair{before, after};
           }
         ) |
         std::ranges::to<std::vector>();
}

///
/// Find the lines around the cursor character of \p position_data, the line it sits on and the line above and below.
/// \return padded area of those lines, nothing if the cursor character has no bounding box
///
[[nodiscard]] auto cursor_lines_area(const reference_ocr::reference_position_data& position_data)
  -> std::optional<data::screen_rect_type>
{
  const auto cursor = position_data.cursor_character_index;
  const auto runs = character_runs(position_data);
  const auto holds_cursor = [&](const auto line)
  { return line.front().index_range.begin <= cursor && cursor < line.back().index_range.end; };
  const auto area_of = [](const auto line)
  {
    return *std::ranges::fold_left_first(
      line | std::views::transform(&character_run::bounding_box),
      [](const auto& a, const auto& b) { return math::surrounding_rect(a, b); }
    );
  };
  const auto lines = runs | std::views::chunk_by(on_same_line) |
                     std::views::transform([&](const auto line) { return std::pair{area_of(line), holds_cursor(line)}; }) |
                     std::ranges::to<std::vector>();
  const auto cursor_line = std::ranges::find_if(lines, [](const auto& line) { return line.second; });
  if(cursor_line == std::ranges::cend(lines))
  {
    return std::nullopt;
  }
  // A reference may be broken over a line break, so the line above and below are taken as well. A line of another
  // column follows in the text too, it does not share a column with the cursor line.
  const auto cursor_area = cursor_line->first;
  const auto same_column = [&](const data::screen_rect_type& area)
  { return math::overlaps(area.horizontal_range(), cursor_area.horizontal_range()); };
  const auto lines_area = std::ranges::fold_left(
    with_neighbours(lines, cursor_line) | std::views::keys | std::views::filter(same_column),
    cursor_area,
    [](const auto& a, const auto& b) { return math::surrounding_rect(a, b); }
  );
  // Padded by half the height of all lines, half the height of the cursor line lost "1. Korinther 6, 20" of a capture.
  return padded(lines_area, lines_area);
}

///
/// Copy \p area of \p image enlarged by \p scale, the pixels in between are interpolated linearly.
/// \pre \p area lies within \p image and is not empty, \p scale is at least 1
/// \return enlarged copy of the area
///
[[nodiscard]] auto enlarged_copy(
  const reference_ocr::pixel_plane_view_type& image, const data::screen_rect_type& area, const double scale
) -> data::pixel_plane_type
{
  assert(!math::empty(area) && scale >= 1.0);
  // A pixel holds red, green, blue and alpha in one byte each, the layout of an rgba8 pixel of boost::gil.
  static_assert(sizeof(data::pixel) == sizeof(boost::gil::rgba8_pixel_t));

  const auto area_width = math::size(area.horizontal_range());
  const auto area_height = math::size(area.vertical_range());
  const auto enlarged_size = [&](const auto size)
  { return static_cast<std::uint32_t>(std::lround(static_cast<double>(size) * scale)); };
  auto result = data::pixel_plane_type{enlarged_size(area_width), enlarged_size(area_height)};

  const auto image_view = boost::gil::interleaved_view(
    image.width(),
    image.height(),
    reinterpret_cast<const boost::gil::rgba8_pixel_t*>(std::to_address(image.begin())),
    static_cast<std::ptrdiff_t>(image.width()) * data::pixel::bytes_per_pixel
  );
  const auto result_view = boost::gil::interleaved_view(
    result.width(),
    result.height(),
    reinterpret_cast<boost::gil::rgba8_pixel_t*>(&result.at(0)),
    static_cast<std::ptrdiff_t>(result.width()) * data::pixel::bytes_per_pixel
  );
  boost::gil::resize_view(
    boost::gil::subimage_view(
      image_view,
      area.origin().x(),
      area.origin().y(),
      static_cast<std::ptrdiff_t>(area_width),
      static_cast<std::ptrdiff_t>(area_height)
    ),
    result_view,
    boost::gil::bilinear_sampler{}
  );
  return result;
}

///
/// Recognize the lines around the cursor character of the earlier recognition again from an enlarged copy of them.
/// \see reference_ocr::enlarged_lines_recognition
/// \return text with character index corresponding to the specified position, the boxes in the coordinates of \p image
///
[[nodiscard]] auto recognize_enlarged_lines(
  const reference_ocr::ocr_engine_list_type& engines,
  const reference_ocr::engine_names& names,
  const reference_ocr::pixel_plane_view_type& image,
  const reference_ocr::position_type position,
  const reference_ocr::enlarged_lines_recognition& algorithm
) -> std::expected<reference_ocr::reference_position_data, reference_ocr::unexpected_ocr_result>
{
  const auto scale = algorithm.scale;
  const auto image_area = data::screen_rect_type{math::coordinates(0, 0), image.width(), image.height()};
  const auto lines_area = cursor_lines_area(algorithm.earlier_recognition);
  const auto area = lines_area ? math::overlap(*lines_area, image_area) : std::nullopt;

  const auto recognize = [&](const data::screen_rect_type& area)
  {
    const auto enlarged = enlarged_copy(image, area, scale);
    const auto origin = area.origin();
    const auto to_enlarged = [&](const auto value, const auto area_origin)
    { return static_cast<data::screen_rect_type::value_type>(std::lround(static_cast<double>(value - area_origin) * scale)); };
    auto result = recognize_just_with_line_recognition(
      engines,
      names,
      reference_ocr::pixel_plane_view_type{enlarged},
      reference_ocr::position_type{to_enlarged(position.x(), origin.x()), to_enlarged(position.y(), origin.y())}
    );
    // The engine reports the boxes in the enlarged copy, the caller expects them in the image.
    const auto to_image = [&](const auto value) { return std::lround(static_cast<double>(value) / scale); };
    const auto to_image_box = [&](const data::screen_rect_type& box)
    {
      return data::screen_rect_type{
        math::coordinates(
          origin.x() + static_cast<data::screen_rect_type::value_type>(to_image(box.origin().x())),
          origin.y() + static_cast<data::screen_rect_type::value_type>(to_image(box.origin().y()))
        ),
        static_cast<std::uint32_t>(to_image(math::size(box.horizontal_range()))),
        static_cast<std::uint32_t>(to_image(math::size(box.vertical_range())))
      };
    };
    if(result)
    {
      std::ranges::for_each(result->character_bounding_boxes, [&](auto& box) { box = box.transform(to_image_box); });
    }
    return result;
  };
  return area && !math::empty(*area) ? recognize(*area) : reference_ocr::reference_position_data{};
}

} // namespace

///
///
auto reference_ocr::run(
  const ocr_engine_list_type& engines,
  const engine_names& names,
  const pixel_plane_view_type& image,
  const position_type position,
  const algorithm_type& algorithm
) -> std::expected<reference_position_data, unexpected_ocr_result>
{
  return util::visit_lambdas(
    algorithm,
    [&]([[maybe_unused]] const paragraph_recognition&)
    { return recognize_with_paragraph_recognition(engines, names, image, position); },
    [&]([[maybe_unused]] const line_recognition&)
    { return recognize_just_with_line_recognition(engines, names, image, position); },
    [&](const enlarged_lines_recognition& enlarged)
    { return recognize_enlarged_lines(engines, names, image, position, enlarged); }
  );
}

///
///
auto reference_ocr::consecutive_characters(
  const reference_position_data& position_data, const position_type position, const math::value_range<std::size_t> index_range
) -> math::value_range<std::size_t>
{
  const auto whole_text = math::value_range<std::size_t>{0u, position_data.text.size()};
  // Without a box per character there is no run and so no gap to measure.
  const auto runs = character_runs(position_data);
  const auto overlaps_index_range = [&](const auto line)
  {
    return math::overlaps(
      math::value_range<std::size_t>{line.front().index_range.begin, line.back().index_range.end}, index_range
    );
  };
  auto gaps = runs | std::views::chunk_by(on_same_line) | std::views::filter(overlaps_index_range) |
              std::views::transform([&](const auto line) { return wide_gaps_in_line(std::span{line}, index_range); }) |
              std::views::join;
  const auto up_to_gap =
    [&](const math::value_range<std::size_t> characters, const std::pair<character_run, character_run>& gap)
  {
    const auto& [before, after] = gap;
    // On the line of the gap the position is placed by its column, on another line by that line.
    const auto gap_line = math::surrounding_rect(before.bounding_box, after.bounding_box).vertical_range();
    const auto on_gap_line = math::contains(gap_line, position.y());
    const auto before_gap =
      on_gap_line ? position.x() < before.bounding_box.horizontal_range().end : position.y() < gap_line.begin;
    const auto behind_gap =
      on_gap_line ? position.x() >= after.bounding_box.horizontal_range().begin : position.y() >= gap_line.end;
    // A position within the gap points at the dropped text itself, none of the characters around belong to it.
    const auto within_gap = math::value_range<std::size_t>{before.index_range.end, before.index_range.end};
    return before_gap   ? math::value_range<std::size_t>{characters.begin, std::min(characters.end, before.index_range.end)}
           : behind_gap ? math::value_range<std::size_t>{std::max(characters.begin, after.index_range.begin), characters.end}
                        : within_gap;
  };
  return std::ranges::fold_left(gaps, whole_text, up_to_gap);
}

} // namespace bibstd::bible
