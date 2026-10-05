#include "test_utils/ocr_capture_data.hpp"

#include <bibstd/bible/reference_ocr.hpp>
#include <bibstd/data/pixel.hpp>
#include <bibstd/data/plane.hpp>
#include <bibstd/data/screen_types.hpp>
#include <bibstd/math/coordinates.hpp>
#include <bibstd/math/rect.hpp>
#include <bibstd/math/value_range.hpp>
#include <bibstd/txt/ocr_engine.hpp>
#include <bibstd/util/ranges.hpp>
#include <bibstd/util/scope_guard.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bibstd::bible
{
namespace
{

using area_type = data::pixel_plane_view_type::area_type;

///
/// \return Capture box as screen rect
///
[[nodiscard]] auto to_rect(const test_utils::capture_box& box) -> data::screen_rect_type
{
  return data::screen_rect_type{
    math::coordinates(box.x, box.y), static_cast<std::uint32_t>(box.width), static_cast<std::uint32_t>(box.height)
  };
}

///
/// \return Screen rect moved by the given offset
///
[[nodiscard]] auto shifted(const data::screen_rect_type& box, const data::screen_coordinates_type offset)
  -> data::screen_rect_type
{
  return data::screen_rect_type{box.origin() + offset, math::size(box.horizontal_range()), math::size(box.vertical_range())};
}

///
/// \return Centre of a capture box, the position a user would point at to hit the word
///
[[nodiscard]] auto centre(const test_utils::capture_box& box) -> reference_ocr::position_type
{
  return {box.x + (box.width / 2), box.y + (box.height / 2)};
}

///
/// OCR engine that replays captured data instead of looking at the image. Recognition honours the
/// subarea the same way a real engine does: only overlapping words are reported and their boxes are
/// relative to the subarea, so that reference_ocr has to shift them back itself.
///
class capture_engine final : public txt::ocr_engine<txt::ocr_engine_tag_layout_analysis>
{
  // Variables
  test_utils::capture_data data_;
  std::optional<area_type> subarea_;

public: // Constants
  static constexpr auto default_name = "capture";

public: // Structors
  explicit capture_engine(test_utils::capture_data data)
    : data_{std::move(data)}
  {
  }

public: // Accessors
  ///
  /// \return Subarea the engine was initialized with last
  ///
  [[nodiscard]] auto last_subarea() const -> const std::optional<area_type>& { return subarea_; }

public: // Overrides
  auto name() const -> name_type override { return name_type{default_name}; }

  auto initialize([[maybe_unused]] pixel_plane_view_type image, std::optional<area_type> subarea) -> void override
  {
    subarea_ = subarea;
  }

  auto recognize() const -> recognition_data override
  {
    const auto offset = reported_offset();
    const auto to_word = [&](const test_utils::capture_text& t) { return word{t.text, shifted(to_rect(t.box), offset)}; };
    const auto to_line = [&](const test_utils::capture_text& t) { return line{t.text, shifted(to_rect(t.box), offset)}; };
    const auto to_paragraph = [&](const test_utils::capture_text& t)
    { return paragraph{t.text, shifted(to_rect(t.box), offset)}; };

    auto result = recognition_data{};
    for(const auto& element : data_.words)
    {
      if(!recognized(element.word.box))
      {
        continue;
      }
      result.emplace_back(
        recognition_data_element{
          .word_data = to_word(element.word),
          .line_data = element.line ? std::optional{to_line(*element.line)} : std::nullopt,
          .paragraph_data = element.paragraph ? std::optional{to_paragraph(*element.paragraph)} : std::nullopt
        }
      );
    }
    return result;
  }

  auto layout_analysis() const -> std::vector<line_layout> override
  {
    auto result = std::vector<line_layout>{};
    for(const auto& layout : data_.layouts)
    {
      result.emplace_back(
        line_layout{
          .line_bounding_box = to_rect(layout.line),
          .paragraph_bounding_box = layout.paragraph ? std::optional{to_rect(*layout.paragraph)} : std::nullopt
        }
      );
    }
    return result;
  }

private: // Implementation
  ///
  /// A real engine recognizes the subarea clipped to the image, so a subarea reaching over an image
  /// edge does not shift the result.
  /// \return Subarea the engine works on, std::nullopt when the whole image is recognized
  ///
  [[nodiscard]] auto clipped_subarea() const -> std::optional<area_type>
  {
    const auto image_box = test_utils::capture_box{
      .width = static_cast<std::int32_t>(data_.width), .height = static_cast<std::int32_t>(data_.height)
    };
    return subarea_ ? math::overlap(*subarea_, area_type{to_rect(image_box)}) : std::nullopt;
  }

  ///
  /// \return Offset turning image coordinates into the coordinates the engine reports its boxes in
  ///
  [[nodiscard]] auto reported_offset() const -> data::screen_coordinates_type
  {
    const auto clipped = clipped_subarea();
    return clipped ? data::screen_coordinates_type{
                       -static_cast<data::screen_rect_type::value_type>(clipped->origin().x()),
                       -static_cast<data::screen_rect_type::value_type>(clipped->origin().y())
                     }
                   : data::screen_coordinates_type{0, 0};
  }

  ///
  /// \return true if the element is part of the recognized area
  ///
  [[nodiscard]] auto recognized(const test_utils::capture_box& box) const -> bool
  {
    if(!subarea_)
    {
      return true;
    }
    const auto clipped = clipped_subarea();
    return clipped && math::overlap(*clipped, area_type{to_rect(box)}).has_value();
  }
};

///
/// OCR engine replaying a capture without its paragraphs, the shape a system engine has.
///
class line_capture_engine final : public txt::ocr_engine<txt::ocr_engine_tag_plain>
{
  // Variables
  capture_engine engine_;

public: // Constants
  static constexpr auto default_name = "lines";

public: // Structors
  explicit line_capture_engine(test_utils::capture_data data)
    : engine_{std::move(data)}
  {
  }

public: // Overrides
  auto name() const -> name_type override { return name_type{default_name}; }

  auto initialize(pixel_plane_view_type image, std::optional<area_type> subarea) -> void override
  {
    engine_.initialize(image, subarea);
  }

  auto recognize() const -> recognition_data override
  {
    auto result = recognition_data{};
    for(const auto& element : engine_.recognize())
    {
      result.emplace_back(recognition_data_element{.word_data = element.word_data, .line_data = element.line_data});
    }
    return result;
  }
};

///
/// OCR engine reporting fixed words, whatever image it is given. It keeps the image it was initialized with last.
///
class fixed_words_engine final : public txt::ocr_engine<txt::ocr_engine_tag_plain>
{
  // Variables
  recognition_data data_;
  data::pixel_plane_type image_;

public: // Constants
  static constexpr auto default_name = "fixed";

public: // Structors
  explicit fixed_words_engine(recognition_data data)
    : data_{std::move(data)}
  {
  }

public: // Accessors
  ///
  /// \return Image the engine was initialized with last
  ///
  [[nodiscard]] auto image() const -> const data::pixel_plane_type& { return image_; }

public: // Overrides
  auto name() const -> name_type override { return name_type{default_name}; }

  auto initialize(pixel_plane_view_type image, [[maybe_unused]] std::optional<area_type> subarea) -> void override
  {
    image_ = data::pixel_plane_type{image.width(), image.height()};
    std::ranges::for_each(
      util::ranges::index_view_to(image.size()), [&](const auto i) { image_.at(i) = std::as_const(image).at(i); }
    );
  }

  auto recognize() const -> recognition_data override { return data_; }
};

///
/// OCR engine without layout analysis support, to reach the paths that require one.
///
class plain_engine final : public txt::ocr_engine<txt::ocr_engine_tag_plain>
{
public: // Constants
  static constexpr auto default_name = "plain";

public: // Overrides
  auto name() const -> name_type override { return name_type{default_name}; }

  auto initialize([[maybe_unused]] pixel_plane_view_type image, [[maybe_unused]] std::optional<area_type> subarea)
    -> void override
  {
  }

  auto recognize() const -> recognition_data override { return recognition_data{}; }
};

///
/// A designed capture with two paragraphs: the first holds two lines, the second holds one. The
/// numbers are round so that the areas the paragraph recognition derives can be written down by hand.
///
///   y= 50  Der Vers Johannes 3,     |
///   y= 80  16 ist bekannt.          | paragraph 1
///   y=120  Ein anderer Absatz.        paragraph 2
///
struct designed_capture final
{
  // Constants
  static constexpr auto line_height = std::int32_t{20};

  // Variables
  test_utils::capture_data data;

  ///
  /// \see designed_capture
  ///
  designed_capture()
  {
    const auto text_of = [](const std::string& text, const std::int32_t x, const std::int32_t y, const std::int32_t width)
    {
      return test_utils::capture_text{
        .text = text, .box = {.x = x, .y = y, .width = width, .height = line_height}
      };
    };
    const auto line1 = text_of("Der Vers Johannes 3,\n", 50, 50, 200);
    const auto line2 = text_of("16 ist bekannt.\n", 50, 80, 200);
    const auto line3 = text_of("Ein anderer Absatz.\n", 50, 120, 200);
    const auto paragraph1 = test_utils::capture_text{
      .text = line1.text + line2.text, .box = {.x = 50, .y = 50, .width = 200, .height = 50}
    };
    const auto paragraph2 = test_utils::capture_text{.text = line3.text, .box = line3.box};

    data = test_utils::capture_data{
      .id = "designed",
      .width = 300,
      .height = 180,
      .layouts =
        {test_utils::capture_layout{.line = line1.box, .paragraph = paragraph1.box},
                  test_utils::capture_layout{.line = line2.box, .paragraph = paragraph1.box},
                  test_utils::capture_layout{.line = line3.box, .paragraph = paragraph2.box}},
      .words = {
                  test_utils::capture_word{.word = text_of("Der", 50, 50, 30), .line = line1, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("Vers", 90, 50, 40), .line = line1, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("Johannes", 140, 50, 80), .line = line1, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("3,", 230, 50, 20), .line = line1, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("16", 50, 80, 20), .line = line2, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("ist", 80, 80, 30), .line = line2, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("bekannt.", 120, 80, 80), .line = line2, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("Ein", 50, 120, 30), .line = line3, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("anderer", 90, 120, 70), .line = line3, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("Absatz.", 170, 120, 70), .line = line3, .paragraph = paragraph2}
      }
    };
  }

  ///
  /// \return Box of the word with the given text
  ///
  [[nodiscard]] auto word(const std::string_view text) const -> test_utils::capture_box
  {
    const auto it = std::ranges::find(data.words, text, [](const auto& w) { return std::string_view{w.word.text}; });
    REQUIRE(it != std::ranges::cend(data.words));
    return it->word.box;
  }

  ///
  /// Move the whole capture, so that the area the paragraph recognition asks for reaches over the image edge.
  ///
  auto move_by(const std::int32_t offset) -> void
  {
    const auto move = [offset](test_utils::capture_box& box)
    {
      box.x -= offset;
      box.y -= offset;
    };
    for(auto& layout : data.layouts)
    {
      move(layout.line);
      if(layout.paragraph)
      {
        move(*layout.paragraph);
      }
    }
    for(auto& element : data.words)
    {
      move(element.word.box);
      if(element.line)
      {
        move(element.line->box);
      }
      if(element.paragraph)
      {
        move(element.paragraph->box);
      }
    }
  }
};

///
/// A designed capture whose paragraph repeats a word of the line above it and words of its own.
/// The word above carries a descender reaching into the area the paragraph recognition asks for,
/// so it is recognized together with the paragraph although it supplies none of its text.
///
///   y= 20  Was hier gilt.            paragraph 1, "gilt." reaching down to y=55
///   y= 50  Der Vers Johannes 3,    |
///   y= 80  16 ist bekannt.         | paragraph 2
///   y=110  Der Vers gilt.          |
///
struct repeated_word_capture final
{
  // Constants
  static constexpr auto line_height = std::int32_t{20};

  // Variables
  test_utils::capture_data data;

  ///
  /// \see repeated_word_capture
  ///
  repeated_word_capture()
  {
    const auto text_of = [](
                           const std::string& text,
                           const std::int32_t x,
                           const std::int32_t y,
                           const std::int32_t width,
                           const std::int32_t height = line_height
                         )
    {
      return test_utils::capture_text{
        .text = text, .box = {.x = x, .y = y, .width = width, .height = height}
      };
    };
    const auto line1 = text_of("Was hier gilt.\n", 50, 20, 200);
    const auto line2 = text_of("Der Vers Johannes 3,\n", 50, 50, 200);
    const auto line3 = text_of("16 ist bekannt.\n", 50, 80, 200);
    const auto line4 = text_of("Der Vers gilt.\n", 50, 110, 200);
    const auto paragraph1 = test_utils::capture_text{.text = line1.text, .box = line1.box};
    const auto paragraph2 = test_utils::capture_text{
      .text = line2.text + line3.text + line4.text, .box = {.x = 50, .y = 50, .width = 200, .height = 80}
    };

    data = test_utils::capture_data{
      .id = "repeated word",
      .width = 300,
      .height = 200,
      .layouts =
        {test_utils::capture_layout{.line = line1.box, .paragraph = paragraph1.box},
                  test_utils::capture_layout{.line = line2.box, .paragraph = paragraph2.box},
                  test_utils::capture_layout{.line = line3.box, .paragraph = paragraph2.box},
                  test_utils::capture_layout{.line = line4.box, .paragraph = paragraph2.box}},
      .words = {
                  test_utils::capture_word{.word = text_of("Was", 50, 20, 30), .line = line1, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("hier", 90, 20, 40), .line = line1, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("gilt.", 140, 20, 50, 35), .line = line1, .paragraph = paragraph1},
                  test_utils::capture_word{.word = text_of("Der", 50, 50, 30), .line = line2, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("Vers", 90, 50, 40), .line = line2, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("Johannes", 140, 50, 80), .line = line2, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("3,", 230, 50, 20), .line = line2, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("16", 50, 80, 20), .line = line3, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("ist", 80, 80, 30), .line = line3, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("bekannt.", 120, 80, 80), .line = line3, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("Der", 50, 110, 30), .line = line4, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("Vers", 90, 110, 50), .line = line4, .paragraph = paragraph2},
                  test_utils::capture_word{.word = text_of("gilt.", 150, 110, 50), .line = line4, .paragraph = paragraph2}
      }
    };
  }

  ///
  /// \return Boxes of all words with the given text, in reading order
  ///
  [[nodiscard]] auto word_boxes(const std::string_view text) const -> std::vector<test_utils::capture_box>
  {
    return data.words | std::views::filter([&](const auto& w) { return w.word.text == text; }) |
           std::views::transform([](const auto& w) { return w.word.box; }) | std::ranges::to<std::vector>();
  }

  ///
  /// \return Box of the word with the given text, the first one if the text is repeated
  ///
  [[nodiscard]] auto word(const std::string_view text) const -> test_utils::capture_box
  {
    const auto it = std::ranges::find(data.words, text, [](const auto& w) { return std::string_view{w.word.text}; });
    REQUIRE(it != std::ranges::cend(data.words));
    return it->word.box;
  }
};

///
/// One capture, the blank image belonging to it and the engines replaying it. The pixels are never
/// looked at, only the dimensions are, because the recognition clips its area to the image.
///
class ocr_driver final
{
  // Variables
  data::pixel_plane_type image_;
  reference_ocr::ocr_engine_list_type engines_;
  // Adding engines may move the list, the engines themselves stay put behind their unique_ptr.
  const capture_engine* capture_engine_{nullptr};

public: // Structors
  explicit ocr_driver(const test_utils::capture_data& capture)
    : image_{capture.width, capture.height}
  {
    auto engine = std::make_unique<capture_engine>(capture);
    capture_engine_ = engine.get();
    engines_.emplace_back(std::move(engine));
  }

public: // Accessors
  ///
  /// \return Engine list, to add further engines or to hand it to reference_ocr directly
  ///
  [[nodiscard]] auto engines() -> reference_ocr::ocr_engine_list_type& { return engines_; }

  ///
  /// \return Blank image with the dimensions of the capture
  ///
  [[nodiscard]] auto image() const -> data::pixel_plane_view_type { return data::pixel_plane_view_type{image_}; }

  ///
  /// \return Subarea the replaying engine was initialized with last
  ///
  [[nodiscard]] auto last_subarea() const -> const std::optional<area_type>& { return capture_engine_->last_subarea(); }

public: // Operations
  ///
  /// \return Engine names driving both algorithms with the replaying engine
  ///
  [[nodiscard]] static auto engine_names() -> reference_ocr::engine_names
  {
    return reference_ocr::engine_names{
      .character_recognition = capture_engine::default_name, .layout_recognition = capture_engine::default_name
    };
  }

  ///
  /// Run the recognition at the given position.
  /// \return Result of \see reference_ocr::run
  ///
  [[nodiscard]] auto run(
    const reference_ocr::position_type position,
    const reference_ocr::algorithm_type& algorithm = reference_ocr::paragraph_recognition{}
  )
  {
    return reference_ocr::run(engines_, engine_names(), image(), position, algorithm);
  }

  ///
  /// Run the recognition at the centre of the given box.
  /// \return Result of \see reference_ocr::run
  ///
  [[nodiscard]] auto run_at(
    const test_utils::capture_box& box, const reference_ocr::algorithm_type& algorithm = reference_ocr::paragraph_recognition{}
  )
  {
    return run(centre(box), algorithm);
  }
};

///
/// Word placed on a line of \see position_data_of, its box spans the whole height of the word.
///
struct placed_word final
{
  std::string text;
  std::int32_t x;
  std::int32_t width;
  std::int32_t height;
};

///
/// Build position data the way reference_ocr reports it: words and lines separated by a space,
/// the box of a word divided evenly among its characters and no box for the separators.
/// \return position data of the lines
///
[[nodiscard]] auto position_data_of(const std::vector<std::vector<placed_word>>& lines)
  -> reference_ocr::reference_position_data
{
  auto result = reference_ocr::reference_position_data{};
  for(const auto& [line_index, line] : lines | std::views::enumerate)
  {
    const auto y = static_cast<std::int32_t>(50 + (40 * line_index));
    for(const auto& [word_index, word] : line | std::views::enumerate)
    {
      if(word_index > 0)
      {
        result.text.push_back(' ');
        result.character_bounding_boxes.emplace_back(std::nullopt);
      }
      const auto character_width = word.width / static_cast<std::int32_t>(word.text.size());
      for(const auto& [character_index, character] : word.text | std::views::enumerate)
      {
        result.text.push_back(character);
        result.character_bounding_boxes.emplace_back(
          data::screen_rect_type{
            math::coordinates(word.x + (static_cast<std::int32_t>(character_index) * character_width), y),
            static_cast<std::uint32_t>(character_width),
            static_cast<std::uint32_t>(word.height)
          }
        );
      }
    }
    result.text.push_back(' ');
    result.character_bounding_boxes.emplace_back(std::nullopt);
  }
  return result;
}

///
/// \return Index range of the first occurrence of the text
///
[[nodiscard]] auto range_of(const reference_ocr::reference_position_data& position_data, const std::string_view text)
  -> math::value_range<std::size_t>
{
  const auto begin = position_data.text.find(text);
  REQUIRE(begin != std::string::npos);
  return math::value_range<std::size_t>{begin, begin + text.size()};
}

///
/// Line of a screenshot as the windows engine read it. A drawn line crossed the "1" of "Psalm 107, 2" and the engine
/// read "07,", \p number places either reading. The engine reported these boxes, "107," widens "07," by the "1".
///
[[nodiscard]] auto psalm_line(const std::string& number, const std::int32_t number_x, const std::int32_t number_width)
  -> std::vector<placed_word>
{
  return {
    {  "saja",       13,           45, 20},
    {   "43,",       68,           33, 20},
    {     "1",      112,           10, 16},
    {     "/",      133,            9, 20},
    {"Jesaja",      149,           70, 20},
    {   "44,",      228,           34, 20},
    {    "22",      272,           27, 16},
    {     "/",      306,           13, 20},
    { "Psalm",      326,           66, 17},
    {  number, number_x, number_width, 20},
    {     "2",      460,           13, 16},
    {     "/",      480,           13, 20},
    {"Klage-",      500,           71, 21}
  };
}

///
/// A designed capture of one line whose engine dropped "Gal 6," between "7,19;" and "13)", unless the gap is closed.
///
///   y=50  Siehe (Joh 7,19; 13) dazu.
///
[[nodiscard]] auto dropped_word_capture(const bool with_gap) -> test_utils::capture_data
{
  const auto text_of = [](const std::string& text, const std::int32_t x, const std::int32_t width)
  {
    return test_utils::capture_text{
      .text = text, .box = {.x = x, .y = 50, .width = width, .height = 20}
    };
  };
  const auto shift = with_gap ? 0 : -46;
  const auto line = text_of("Siehe (Joh 7,19; 13) dazu.\n", 50, 300);
  const auto word_of = [&](const std::string& text, const std::int32_t x, const std::int32_t width)
  { return test_utils::capture_word{.word = text_of(text, x, width), .line = line, .paragraph = line}; };
  return test_utils::capture_data{
    .id = "dropped word",
    .width = 400,
    .height = 120,
    .layouts = {test_utils::capture_layout{.line = line.box, .paragraph = line.box}},
    .words = {
                word_of("Siehe", 50, 50),
                word_of("(Joh", 108, 40),
                word_of("7,19;", 156, 50),
                word_of("13)", 260 + shift, 30),
                word_of("dazu.", 298 + shift, 50)
    }
  };
}

///
/// \return Consecutive characters around the first character of \p word, only gaps bordering \p reference count
///
[[nodiscard]] auto consecutive_characters_at(
  const reference_ocr::reference_position_data& position_data, const std::string_view reference, const std::string_view word
) -> math::value_range<std::size_t>
{
  const auto& box = position_data.character_bounding_boxes.at(range_of(position_data, word).begin);
  REQUIRE(box.has_value());
  return reference_ocr::consecutive_characters(position_data, box->center(), range_of(position_data, reference));
}

///
/// \return Index range from the first character of the text up to the end of \p word
///
[[nodiscard]] auto up_to(const reference_ocr::reference_position_data& position_data, const std::string_view word)
  -> math::value_range<std::size_t>
{
  return math::value_range<std::size_t>{0u, range_of(position_data, word).end};
}

///
/// \return Index range from the beginning of \p word up to the end of the text
///
[[nodiscard]] auto from(const reference_ocr::reference_position_data& position_data, const std::string_view word)
  -> math::value_range<std::size_t>
{
  return math::value_range<std::size_t>{range_of(position_data, word).begin, position_data.text.size()};
}

///
/// \return Index range of the whole text
///
[[nodiscard]] auto whole_text(const reference_ocr::reference_position_data& position_data) -> math::value_range<std::size_t>
{
  return math::value_range<std::size_t>{0u, position_data.text.size()};
}

} // namespace

TEST_CASE("reference_ocr resolves the position to a character of the paragraph", "[bible]")
{
  const auto capture = designed_capture{};
  auto driver = ocr_driver{capture.data};

  SECTION("a position on a word of the first paragraph")
  {
    const auto result = driver.run_at(capture.word("Johannes"));
    REQUIRE(result.has_value());

    // Both lines belong to one paragraph, so the whole paragraph is returned, the line break as a space.
    CHECK(result->text == "Der Vers Johannes 3, 16 ist bekannt. ");
    CHECK(result->character_bounding_boxes.size() == result->text.size());

    // The cursor character has to land inside the word that was pointed at.
    const auto word_begin = result->text.find("Johannes");
    REQUIRE(word_begin != std::string::npos);
    CHECK(result->cursor_character_index >= word_begin);
    CHECK(result->cursor_character_index < word_begin + std::string_view{"Johannes"}.size());
  }
  SECTION("a position on a word of the second paragraph")
  {
    const auto result = driver.run_at(capture.word("Absatz."));
    REQUIRE(result.has_value());
    CHECK(result->text == "Ein anderer Absatz. ");
  }
  SECTION("a position outside every line")
  {
    const auto result = driver.run(math::coordinates(290, 175));
    REQUIRE(result.has_value());
    CHECK(result->text.empty());
    CHECK(result->character_bounding_boxes.empty());
  }
  SECTION("recognition without layout analysis")
  {
    const auto result = driver.run_at(capture.word("Johannes"), reference_ocr::line_recognition{});
    REQUIRE(result.has_value());

    // Without a subarea the whole image is recognized, but the reported element is the same.
    CHECK(result->text == "Der Vers Johannes 3, 16 ist bekannt. ");
    CHECK(result->character_bounding_boxes.size() == result->text.size());
  }
}

TEST_CASE("reference_ocr keeps the lines around the position when the engine reports no paragraph", "[bible]")
{
  // A reference may be broken over a line break, so a line only engine would cut it in half.
  const auto capture = designed_capture{};
  auto driver = ocr_driver{capture.data};
  driver.engines().emplace_back(std::make_unique<line_capture_engine>(capture.data));

  auto names = ocr_driver::engine_names();
  auto algorithm = reference_ocr::algorithm_type{reference_ocr::paragraph_recognition{}};
  names.character_recognition = line_capture_engine::default_name;

  SECTION("both lines of the recognized paragraph")
  {
    const auto result =
      reference_ocr::run(driver.engines(), names, driver.image(), centre(capture.word("Johannes")), algorithm);
    REQUIRE(result.has_value());
    CHECK(result->text == "Der Vers Johannes 3, 16 ist bekannt. ");
    CHECK(result->character_bounding_boxes.size() == result->text.size());
  }
  SECTION("without layout analysis only the line above and below")
  {
    algorithm = reference_ocr::line_recognition{};
    const auto result = reference_ocr::run(driver.engines(), names, driver.image(), centre(capture.word("anderer")), algorithm);
    REQUIRE(result.has_value());

    // The whole image is recognized here, so the first line stays out of the text.
    CHECK(result->text == "16 ist bekannt. Ein anderer Absatz. ");
  }
}

TEST_CASE("reference_ocr widens the recognition area to the paragraph", "[bible]")
{
  const auto capture = designed_capture{};
  auto driver = ocr_driver{capture.data};

  SECTION("a position on the first line of a two line paragraph")
  {
    std::ignore = driver.run_at(capture.word("Der"));
    REQUIRE(driver.last_subarea().has_value());
    const auto& area = *driver.last_subarea();

    // The next line of the same paragraph is taken in, then half a line height is added as padding.
    CHECK(area.origin().x() == 40);
    CHECK(area.origin().y() == 40);
    CHECK(math::size(area.horizontal_range()) == 220);
    CHECK(math::size(area.vertical_range()) == 70);
  }
  SECTION("a position on a paragraph of a single line")
  {
    std::ignore = driver.run_at(capture.word("Ein"));
    REQUIRE(driver.last_subarea().has_value());
    const auto& area = *driver.last_subarea();

    // There is no neighbouring line of the same paragraph, so only the padding is added.
    CHECK(area.origin().x() == 40);
    CHECK(area.origin().y() == 110);
    CHECK(math::size(area.horizontal_range()) == 220);
    CHECK(math::size(area.vertical_range()) == 40);
  }
}

TEST_CASE("reference_ocr reports character boxes in image coordinates", "[bible]")
{
  // The character recognition runs on a subarea and reports boxes relative to it, so the result is
  // only usable after reference_ocr shifted them back into the coordinate system of the image.
  const auto capture = designed_capture{};
  auto driver = ocr_driver{capture.data};

  const auto johannes = capture.word("Johannes");
  const auto result = driver.run_at(johannes);
  REQUIRE(result.has_value());
  REQUIRE(result->cursor_character_index < result->character_bounding_boxes.size());

  const auto& cursor_box = result->character_bounding_boxes.at(result->cursor_character_index);
  REQUIRE(cursor_box.has_value());
  CHECK(math::overlap(*cursor_box, to_rect(johannes)).has_value());
}

TEST_CASE("reference_ocr reports character boxes of a paragraph at the image edge", "[bible]")
{
  // The padding the paragraph recognition adds reaches over the image edge here, so the recognized
  // area is clipped. The boxes are relative to the clipped area and must still come back in image
  // coordinates.
  auto capture = designed_capture{};
  capture.move_by(46);
  auto driver = ocr_driver{capture.data};

  const auto johannes = capture.word("Johannes");
  const auto result = driver.run_at(johannes);
  REQUIRE(result.has_value());

  // The area asked for starts above the image, so the engine sees it clipped to the image edge.
  REQUIRE(driver.last_subarea().has_value());
  CHECK(driver.last_subarea()->origin().x() == 0);
  CHECK(driver.last_subarea()->origin().y() == 0);

  // The first character of the paragraph is the first character of its first word, so its box
  // pins the coordinate system the boxes are reported in.
  REQUIRE_FALSE(result->character_bounding_boxes.empty());
  const auto& first_box = result->character_bounding_boxes.front();
  REQUIRE(first_box.has_value());
  CHECK(first_box->origin().x() == capture.data.words.front().word.box.x);
  CHECK(first_box->origin().y() == capture.data.words.front().word.box.y);

  REQUIRE(result->cursor_character_index < result->character_bounding_boxes.size());
  const auto& cursor_box = result->character_bounding_boxes.at(result->cursor_character_index);
  REQUIRE(cursor_box.has_value());
  CHECK(math::overlap(*cursor_box, to_rect(johannes)).has_value());
}

TEST_CASE("reference_ocr keeps a word of a neighbouring line out of the text it did not supply", "[bible]")
{
  // The recognized area reaches a bit beyond the paragraph, so words of the lines around it are
  // recognized too. Such a word must not claim the characters of a word of the same spelling
  // inside the text: it would take the character positions of the words following it with it.
  const auto capture = repeated_word_capture{};
  auto driver = ocr_driver{capture.data};
  driver.engines().emplace_back(std::make_unique<line_capture_engine>(capture.data));

  auto names = ocr_driver::engine_names();
  auto algorithm = reference_ocr::algorithm_type{reference_ocr::paragraph_recognition{}};
  SECTION("an engine reporting paragraphs")
  {
    names.character_recognition = capture_engine::default_name;
  }
  SECTION("an engine reporting lines only")
  {
    names.character_recognition = line_capture_engine::default_name;
  }

  const auto bekannt = capture.word("bekannt.");
  const auto result = reference_ocr::run(driver.engines(), names, driver.image(), centre(bekannt), algorithm);
  REQUIRE(result.has_value());
  CHECK(result->text == "Der Vers Johannes 3, 16 ist bekannt. Der Vers gilt. ");
  REQUIRE(result->character_bounding_boxes.size() == result->text.size());

  // The cursor character has to land inside the word that was pointed at, it sits on the line
  // between the two lines the repeated word could have dragged the character positions across.
  const auto word_begin = result->text.find("bekannt.");
  REQUIRE(word_begin != std::string::npos);
  CHECK(result->cursor_character_index >= word_begin);
  CHECK(result->cursor_character_index < word_begin + std::string_view{"bekannt."}.size());

  // The repeated word carries the box of the word inside the paragraph, not the one above it.
  const auto repeated_begin = result->text.rfind("gilt.");
  REQUIRE(repeated_begin != std::string::npos);
  const auto& repeated_box = result->character_bounding_boxes.at(repeated_begin);
  REQUIRE(repeated_box.has_value());
  CHECK(math::overlap(*repeated_box, to_rect(capture.data.words.back().word.box)).has_value());

  // A word repeated within the text takes the occurrence belonging to it, so the boxes of the two
  // occurrences stay on the line each of them was recognized on.
  const auto boxes_of = capture.word_boxes("Vers");
  REQUIRE(boxes_of.size() == 2);
  const auto first_begin = result->text.find("Vers");
  const auto second_begin = result->text.rfind("Vers");
  REQUIRE(first_begin != std::string::npos);
  REQUIRE(first_begin != second_begin);
  const auto& first_box = result->character_bounding_boxes.at(first_begin);
  const auto& second_box = result->character_bounding_boxes.at(second_begin);
  REQUIRE(first_box.has_value());
  REQUIRE(second_box.has_value());
  CHECK(math::overlap(*first_box, to_rect(boxes_of.front())).has_value());
  CHECK(math::overlap(*second_box, to_rect(boxes_of.back())).has_value());
}

TEST_CASE("reference_ocr reports no position data when the pointed at line stays unlocated", "[bible]")
{
  // An engine may report a word that is not part of the text of its own line. Without a single
  // located character of the line that was pointed at there is no cursor index, and the closest
  // one is the first character of the text, a reference the user never pointed at.
  auto capture = designed_capture{};
  const auto absatz = capture.word("Absatz.");
  for(auto& element : capture.data.words)
  {
    if(element.word.box.y == absatz.y)
    {
      element.word.text = "###";
    }
  }
  auto driver = ocr_driver{capture.data};

  const auto result = driver.run_at(absatz);
  REQUIRE(result.has_value());
  CHECK(result->text.empty());
  CHECK(result->character_bounding_boxes.empty());
}

TEST_CASE("reference_ocr rejects unusable engine setups", "[bible]")
{
  const auto capture = designed_capture{};
  auto driver = ocr_driver{capture.data};
  const auto position = centre(capture.word("Johannes"));

  auto names = ocr_driver::engine_names();
  auto algorithm = reference_ocr::algorithm_type{reference_ocr::paragraph_recognition{}};

  SECTION("no engine with the requested character recognition name")
  {
    names.character_recognition = "not the name of any engine";
  }
  SECTION("no engine for the layout recognition")
  {
    names.layout_recognition = std::nullopt;
  }
  SECTION("an engine without layout analysis support for the paragraph recognition")
  {
    driver.engines().emplace_back(std::make_unique<plain_engine>());
    names.layout_recognition = plain_engine::default_name;
  }
  SECTION("an undefined engine")
  {
    driver.engines().emplace_back(std::monostate{});
    algorithm = reference_ocr::line_recognition{};
    names.character_recognition = "Undefined";
  }

  const auto result = reference_ocr::run(driver.engines(), names, driver.image(), position, algorithm);
  REQUIRE_FALSE(result.has_value());
  CHECK(result.error() == reference_ocr::unexpected_ocr_result::error);
}

TEST_CASE("capture data survives a write and read cycle", "[bible]")
{
  const auto path = std::filesystem::temp_directory_path() / "bibstd_test_capture.ocr";
  const auto guard = util::scope_guard{[&] { std::filesystem::remove(path); }};

  const auto capture = designed_capture{};
  REQUIRE(test_utils::write_capture(path, capture.data));

  SECTION("everything the capture holds comes back")
  {
    const auto read = test_utils::read_capture(path);
    REQUIRE(read.has_value());
    CHECK(read->id == path.stem().string());
    CHECK(read->width == capture.data.width);
    CHECK(read->height == capture.data.height);

    REQUIRE(read->layouts.size() == capture.data.layouts.size());
    for(const auto& [expected, actual] : std::views::zip(capture.data.layouts, read->layouts))
    {
      CHECK(actual.line == expected.line);
      CHECK(actual.paragraph == expected.paragraph);
    }

    REQUIRE(read->words.size() == capture.data.words.size());
    for(const auto& [expected, actual] : std::views::zip(capture.data.words, read->words))
    {
      // The line breaks of the line and paragraph texts have to survive the escaping.
      CHECK(actual.word == expected.word);
      CHECK(actual.line == expected.line);
      CHECK(actual.paragraph == expected.paragraph);
    }
  }
  SECTION("a capture killed mid write is rejected")
  {
    // Cutting the file leaves a record without its numbers, which is what a killed capture leaves behind.
    const auto size = std::filesystem::file_size(path);
    REQUIRE(size > 20);
    std::filesystem::resize_file(path, size - 20);
    CHECK_FALSE(test_utils::read_capture(path).has_value());
  }
  SECTION("a missing file is no capture")
  {
    CHECK_FALSE(test_utils::read_capture(path.parent_path() / "no_such_capture.ocr").has_value());
  }
}

TEST_CASE("reference_ocr handles captured screenshots", "[bible]")
{
  // The captures are produced from local screenshots and are not part of the repository,
  // \see bibstd_test/res/ocr/README.md.
  const auto captures = test_utils::read_captures(std::filesystem::path{BIBSTD_TEST_OCR_DIR});
  if(captures.empty())
  {
    SKIP(std::format("no ocr captures in {}", BIBSTD_TEST_OCR_DIR));
  }

  for(const auto& capture : captures)
  {
    INFO(std::format("capture: {}", capture.id));
    const auto image_area = data::screen_rect_type{math::coordinates(0, 0), capture.width, capture.height};

    // run() re-initializes the engine on every call, so one driver serves the whole capture.
    auto driver = ocr_driver{capture};
    driver.engines().emplace_back(std::make_unique<line_capture_engine>(capture));

    // The characters are recognized by the engine of the system, which reports no paragraphs. Only
    // the layout analysis contributes them, so both shapes have to come back with the same quality.
    for(const auto& character_recognition : {capture_engine::default_name, line_capture_engine::default_name})
    {
      INFO(std::format("character recognition: {}", character_recognition));
      auto names = ocr_driver::engine_names();
      auto algorithm = reference_ocr::algorithm_type{reference_ocr::paragraph_recognition{}};
      names.character_recognition = character_recognition;

      for(const auto& element : capture.words)
      {
        INFO(std::format("word: \"{}\"", element.word.text));
        const auto result = reference_ocr::run(driver.engines(), names, driver.image(), centre(element.word.box), algorithm);
        REQUIRE(result.has_value());

        // One box per character, so that the caller can address every character of the text.
        REQUIRE(result->character_bounding_boxes.size() == result->text.size());
        if(result->text.empty())
        {
          continue;
        }
        REQUIRE(result->cursor_character_index < result->text.size());

        // Every reported box has been shifted back into the coordinate system of the image.
        const auto boxes_within_image = std::ranges::all_of(
          result->character_bounding_boxes, [&](const auto& box) { return !box || math::overlap(image_area, *box).has_value(); }
        );
        CHECK(boxes_within_image);

        // The character the cursor resolved to belongs to the word that was pointed at.
        const auto& cursor_box = result->character_bounding_boxes.at(result->cursor_character_index);
        REQUIRE(cursor_box.has_value());
        CHECK(math::overlap(*cursor_box, to_rect(element.word.box)).has_value());
      }
    }
  }
}

TEST_CASE("reference_ocr ends the consecutive characters at a gap of dropped text", "[bible]")
{
  SECTION("a digit missing in front of a number")
  {
    const auto data = position_data_of({psalm_line("07,", 417, 33)});
    CHECK(consecutive_characters_at(data, "Psalm 07, 2", "Psalm") == up_to(data, "Psalm"));
    CHECK(consecutive_characters_at(data, "Psalm 07, 2", "07,") == from(data, "07,"));
  }
  SECTION("a word missing between two passages")
  {
    // The line the paragraph recognition cropped, "Gal 6," was dropped.
    const auto data = position_data_of({
      {{"zes?", 6, 48, 20}, {"(Joh", 63, 28, 16}, {"7,19;", 98, 33, 15}, {"13)", 182, 20, 16}}
    });
    CHECK(consecutive_characters_at(data, "Joh 7,19; 13", "Joh") == up_to(data, "7,19;"));
    CHECK(consecutive_characters_at(data, "Joh 7,19; 13", "13)") == from(data, "13)"));
  }
  SECTION("a word missing on a line of three words")
  {
    // The boxes of a browser capture, "Gal" was dropped and only two gaps remain.
    const auto data = position_data_of({
      {{"(Joh", 9, 36, 20}, {"7,19;", 52, 45, 18}, {"6,13)", 138, 45, 20}}
    });
    CHECK(consecutive_characters_at(data, "Joh 7,19; 6,13", "6,13)") == from(data, "6,13)"));

    // Pointing at the dropped "Gal" itself, between "7,19;" ending at x=97 and "6,13)" beginning at x=138, none of
    // the characters around belong to it.
    const auto within_gap = reference_ocr::consecutive_characters(data, {115, 60}, range_of(data, "Joh 7,19; 6,13"));
    CHECK(math::empty(within_gap));
  }
  SECTION("a gap on the line a reference continues on")
  {
    const auto data = position_data_of({
      {{"Gott", 20, 40, 20}, {"selbst", 70, 60, 20},  {"(Joh", 140, 40, 20}, {"7,19;", 190, 50, 20}},
      { {"Gal", 20, 30, 20},  {"13).", 110, 40, 20}, {"Darum", 160, 60, 20},   {"ist", 230, 30, 20}}
    });
    CHECK(consecutive_characters_at(data, "Joh 7,19; Gal 13", "Joh") == up_to(data, "Gal"));
    CHECK(consecutive_characters_at(data, "Joh 7,19;", "Joh") == whole_text(data));
  }
  SECTION("a gap on a line followed by a line of wider spacing")
  {
    // The spacing of the following line must not hide the gap.
    const auto data = position_data_of({
      {{"Siehe", 10, 50, 20}, {"(Joh", 68, 40, 20}, {"7,19;", 116, 50, 20}, {"13)", 200, 30, 20}},
      {{"Im", 10, 20, 20}, {"Alten", 70, 50, 20}, {"Testament", 160, 90, 20}, {"wird", 290, 40, 20}, {"das", 370, 30, 20}}
    });
    CHECK(consecutive_characters_at(data, "Joh 7,19; 13", "Joh") == up_to(data, "7,19;"));
  }
  SECTION("a gap on each side")
  {
    const auto data = position_data_of({
      {{"Siehe", 10, 50, 20}, {"(Joh", 68, 40, 20}, {"7,19;", 116, 50, 20}, {"8,1;", 200, 40, 20}, {"13)", 280, 30, 20}}
    });
    CHECK(consecutive_characters_at(data, "Joh 7,19; 8,1; 13", "8,1;") == range_of(data, "8,1;"));
  }
}

TEST_CASE("reference_ocr reports the characters of regularly spaced text as consecutive", "[bible]")
{
  SECTION("words spaced as usual")
  {
    const auto data = position_data_of({psalm_line("107,", 402, 48)});
    CHECK(consecutive_characters_at(data, "Psalm 107, 2", "107,") == whole_text(data));
  }
  SECTION("justified text spacing every word wide")
  {
    const auto data = position_data_of({
      {{"Im", 10, 30, 20}, {"Alten", 70, 60, 20}, {"Testament", 160, 110, 20}, {"wird", 300, 50, 20}}
    });
    CHECK(consecutive_characters_at(data, "Alten Testament", "Alten") == whole_text(data));
  }
  SECTION("a wide gap away from the reference")
  {
    const auto data = position_data_of({psalm_line("07,", 417, 33)});
    CHECK(consecutive_characters_at(data, "Jesaja 44, 22", "44,") == whole_text(data));
  }
  SECTION("a line of three words")
  {
    const auto data = position_data_of({
      {{"(Joh", 9, 36, 20}, {"7,19;", 52, 45, 18}, {"Gal", 108, 27, 20}}
    });
    CHECK(consecutive_characters_at(data, "Joh 7,19; Gal", "Joh") == whole_text(data));
  }
  SECTION("a line beginning right of where the line before ends")
  {
    // Only the height tells the lines apart, the distance between them is no gap.
    const auto data = position_data_of({
      {    {"Er", 10, 20, 20}, {"sagt", 40, 40, 20},  {"(Joh", 90, 40, 20}},
      {{"3,16)", 300, 50, 20}, {"und", 360, 30, 20}, {"geht", 400, 40, 20}}
    });
    CHECK(consecutive_characters_at(data, "Joh 3,16", "Joh") == whole_text(data));
  }
  SECTION("a line of too few words to know its spacing")
  {
    const auto data = position_data_of({
      {{"Joh", 10, 30, 20}, {"3,16", 120, 40, 20}}
    });
    CHECK(consecutive_characters_at(data, "Joh 3,16", "3,16") == whole_text(data));
  }
  SECTION("position data without a box per character")
  {
    // Without a box per character there is no gap to measure.
    auto data = position_data_of({psalm_line("07,", 417, 33)});
    data.character_bounding_boxes.pop_back();
    CHECK(consecutive_characters_at(data, "Psalm 07, 2", "07,") == whole_text(data));
  }
}

TEST_CASE("reference_ocr measures the gaps in the area the paragraph recognition cropped", "[bible]")
{
  // The engine reports the boxes relative to the cropped area, the gap has to survive shifting them back.
  const auto with_gap = GENERATE(true, false);
  INFO(std::format("with gap: {}", with_gap));
  const auto capture = dropped_word_capture(with_gap);
  auto driver = ocr_driver{capture};
  driver.engines().emplace_back(std::make_unique<line_capture_engine>(capture));

  auto names = ocr_driver::engine_names();
  names.character_recognition = line_capture_engine::default_name;
  const auto result = reference_ocr::run(
    driver.engines(), names, driver.image(), centre(capture.words.at(1).word.box), reference_ocr::paragraph_recognition{}
  );
  REQUIRE(result.has_value());
  CHECK(result->text == "Siehe (Joh 7,19; 13) dazu. ");
  CHECK(
    consecutive_characters_at(*result, "Joh 7,19; 13", "Joh") == (with_gap ? up_to(*result, "7,19;") : whole_text(*result))
  );
}

TEST_CASE("reference_ocr reads the lines around the position again from an enlarged copy", "[bible]")
{
  // The whole image read "rühmst 48, 1;" and dropped "(Jes" in front of "48,". A line of the other column is above.
  auto position_data = position_data_of({
    {{"Siehe", 400, 60, 20}},
    {{"rühmst", 100, 60, 20}, {"48,", 200, 30, 20}, {"1;", 240, 20, 20}},
    {{"und", 100, 40, 20}, {"den", 150, 40, 20}}
  });
  position_data.cursor_character_index = range_of(position_data, "48,").begin;
  // The line of the cursor and the line below span x=100..260 and y=90..150, half their height pads them to the area
  // x=70..290 and y=60..180. The line of the other column stays out.
  const auto to_enlarged = [](const std::int32_t x, const std::int32_t y, const std::uint32_t width)
  { return data::screen_rect_type{math::coordinates((x - 70) * 2, (y - 60) * 2), width * 2, 40u}; };
  const auto line = txt::ocr_engine<>::line{"(Jes 48,", to_enlarged(160, 90, 70)};
  auto engine = std::make_unique<fixed_words_engine>(txt::ocr_engine<>::recognition_data{
    {.word_data = {"(Jes", to_enlarged(160, 90, 32)}, .line_data = line},
    { .word_data = {"48,", to_enlarged(200, 90, 30)}, .line_data = line}
  });
  const auto& engine_ref = *engine;
  auto engines = reference_ocr::ocr_engine_list_type{};
  engines.emplace_back(std::move(engine));

  // A horizontal gradient, the red of a pixel is its column less 50.
  auto image = data::pixel_plane_type{480, 200};
  std::ranges::for_each(
    util::ranges::index_view_to(image.size()),
    [&](const auto i) { image.at(i) = data::pixel{.red = static_cast<std::uint8_t>((i % 480) - 50), .alpha = 255}; }
  );

  const auto jes = reference_ocr::position_type{178, 100};
  const auto result = reference_ocr::run(
    engines,
    {.character_recognition = fixed_words_engine::default_name},
    data::pixel_plane_view_type{image},
    jes,
    reference_ocr::enlarged_lines_recognition{.earlier_recognition = position_data, .scale = 2.0}
  );
  REQUIRE(result.has_value());

  SECTION("the engine reads the padded lines of the column enlarged")
  {
    const auto& enlarged = engine_ref.image();
    CHECK(enlarged.width() == 440);
    CHECK(enlarged.height() == 240);
    // The first and the last enlarged pixel of a row lie on the first and the last column of the area, the pixels in
    // between rise with the gradient.
    const auto first_row = util::ranges::index_view_to(enlarged.width()) |
                           std::views::transform([&](const auto x) { return static_cast<int>(enlarged.at(x).red); });
    CHECK(first_row.front() == 20);
    CHECK(first_row.back() == 239);
    CHECK(std::ranges::is_sorted(first_row));
  }
  SECTION("the characters are reported in the coordinates of the image")
  {
    CHECK(result->text == "(Jes 48, ");
    CHECK(result->cursor_character_index == range_of(*result, "(Jes").begin + 2);
    REQUIRE(result->character_bounding_boxes.front().has_value());
    CHECK(*result->character_bounding_boxes.front() == data::screen_rect_type{math::coordinates(160, 90), 8u, 20u});
  }
}

} // namespace bibstd::bible
