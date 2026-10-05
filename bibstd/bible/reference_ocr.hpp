#pragma once

#include "bibstd/data/screen_types.hpp"
#include "bibstd/math/value_range.hpp"
#include "bibstd/txt/ocr_engine.hpp"

#include <expected>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace bibstd::bible
{

///
/// This class initializes ocr engines and provides a set of algorithms
/// to find a bible reference within a provided image.
///
struct reference_ocr final
{
  // Typedefs
  using ocr_engine_list_type = std::vector<txt::ocr_engine_uptr_variant_type>;
  using pixel_plane_view_type = data::pixel_plane_view_type;
  using position_type = data::screen_coordinates_type;

  ///
  /// This struct contains OCR data for recognized reference position:
  /// Text of the relevant part of the recognized area, an index belonging to the
  /// character data closest to the reference position and the bounding box of every
  /// character of the text. The bounding boxes are given in the coordinate system of
  /// the recognized image. For characters no bounding box could be determined for,
  /// std::nullopt is set.
  ///
  struct reference_position_data final
  {
    // Typedefs
    using bounding_box_type = data::screen_rect_type;

    // Variables
    std::string text;
    std::size_t cursor_character_index{0};
    std::vector<std::optional<bounding_box_type>> character_bounding_boxes;
  };

  ///
  /// Enum containing all unexpected OCR results.
  ///
  enum class unexpected_ocr_result
  {
    error
  };

  ///
  /// Algorithm recognizing the lines around the position within the paragraph the layout recognition finds there. Only
  /// that area is recognized, which makes a slow engine fast enough.
  ///
  struct paragraph_recognition final
  {};

  ///
  /// Algorithm recognizing the whole image and taking the line of the position together with the line above and below.
  ///
  struct line_recognition final
  {};

  ///
  /// Algorithm recognizing the lines around the cursor character of an earlier recognition again from a copy of them
  /// enlarged by \p scale. Engines drop small text they read once it is larger. Only the line of the cursor and the line
  /// above and below in its column are enlarged, so this is fast. \p scale has to be at least 1.
  ///
  struct enlarged_lines_recognition final
  {
    reference_position_data earlier_recognition;
    double scale{1.0};
  };

  using algorithm_type = std::variant<paragraph_recognition, line_recognition, enlarged_lines_recognition>;

  ///
  /// Names of the engines recognizing the text. The layout recognition is only needed by the paragraph recognition.
  ///
  struct engine_names final
  {
    std::string character_recognition;
    std::optional<std::string> layout_recognition;
  };

  // Operations
  ///
  /// Recognize the text around \p position in \p image with \p algorithm.
  /// \return reference position data with the bounding boxes in the coordinates of \p image or unexpected result value
  ///
  [[nodiscard]] static auto run(
    const ocr_engine_list_type& engines,
    const engine_names& names,
    const pixel_plane_view_type& image,
    position_type position,
    const algorithm_type& algorithm
  ) -> std::expected<reference_position_data, unexpected_ocr_result>;

  ///
  /// Find the characters around \p position that follow each other as closely as the spacing of their line allows.
  /// Engines leave out text they cannot read without a trace in the text, only a wider gap between the remaining boxes
  /// stays. Text read across such a gap may mean something else. Only gaps bordering the text in \p index_range count.
  /// \return index range around \p position up to the gaps next to it, the whole text if no gap borders
  /// \p index_range, an empty range if \p position lies within a gap
  ///
  [[nodiscard]] static auto consecutive_characters(
    const reference_position_data& position_data, position_type position, math::value_range<std::size_t> index_range
  ) -> math::value_range<std::size_t>;
};

} // namespace bibstd::bible
