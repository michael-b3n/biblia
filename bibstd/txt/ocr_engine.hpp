#pragma once

#include "bibstd/meta/for_each.hpp"
#include "bibstd/util/screen_types.hpp"

#include <memory>
#include <string>
#include <variant>
#include <vector>

// Forward declarations
namespace bibstd::system
{
class ocr_engine_system;
} // namespace bibstd::system
namespace bibstd::txt
{
class ocr_engine_tesseract;
} // namespace bibstd::txt

namespace bibstd::txt
{

///
/// Helper text resolution tag struct of an OCR engine.
///
template<typename T>
struct ocr_engine_resolution_tag final
{
  using underlying_type = T;
};

///
/// Tag for OCR engine indicating line recognition support.
/// The OCR engine supports line and word recognition including bounding box information.
/// No layout analysis or other advanced features are supported.
///
struct ocr_engine_tag_plain
{
private: // Typedefs
  template<typename Tag>
  struct ocr_t final
  {
    std::string text;
    util::screen_rect_type bounding_box;
  };

public: // Typedefs
  using bounding_box_type = decltype(ocr_t<void>::bounding_box);
  using word = ocr_t<struct word_tag>;
  using line = ocr_t<struct line_tag>;
  using resolution_tags = meta::for_each_t<std::variant<word, line>, ocr_engine_resolution_tag>;

protected: // Typedefs
  ///
  /// Recognition data element with word data, and optional line data.
  /// If the word does not belong to a line, std::nullopt is set as the line data.
  ///
  struct recognition_data_element final
  {
    word word_data;
    std::optional<line> line_data;
  };
};

///
/// Tag for OCR engine indicating paragraph recognition support.
/// The OCR engine supports paragraph, line, and word recognition including bounding box information.
/// The paragraph type comes with layout analysis support.
///
struct ocr_engine_tag_layout_analysis
{
  // Typedefs
  using bounding_box_type = ocr_engine_tag_plain::bounding_box_type;
  using word = ocr_engine_tag_plain::word;
  using line = ocr_engine_tag_plain::line;
  struct paragraph final
  {
    std::string text;
    util::screen_rect_type bounding_box;
  };
  using resolution_tags = meta::for_each_t<std::variant<word, line, paragraph>, ocr_engine_resolution_tag>;

protected: // Typedefs
  ///
  /// Line bounding box data including paragraph bounding box.
  ///
  struct line_layout final
  {
    bounding_box_type line_bounding_box;
    std::optional<bounding_box_type> paragraph_bounding_box;
  };

  ///
  /// Recognition data element with word data, and optional line and paragraph data.
  /// If the word does not belong to a line or paragraph, std::nullopt is set as the line or paragraph data.
  ///
  struct recognition_data_element final
  {
    word word_data;
    std::optional<line> line_data;
    std::optional<paragraph> paragraph_data;
  };

public: // Operations
  ///
  /// Run analyze layout on image and list all bounding boxes
  /// corresponding to the provided resolution. If no character recognition
  /// is required, this method is much more efficient.
  /// \return list of bounding boxes corresponding to resolution tag
  ///
  virtual auto layout_analysis() const -> std::vector<line_layout> = 0;
};

///
/// Virtual OCR engine class.
/// \tparam EngineTag specifies the capabilities
/// (resolution level line or paragraph for layout analysis) of the OCR engine.
///
template<typename EngineTag = ocr_engine_tag_plain>
class ocr_engine : public EngineTag
{
protected:
  template<typename ResolutionType>
  using tag = ocr_engine_resolution_tag<ResolutionType>;

public: // Typedefs
  using uptr_type = std::unique_ptr<ocr_engine<EngineTag>>;
  using name_type = std::string;
  using pixel_plane_view_type = util::pixel_plane_view_type;

  using resolution_tags = typename EngineTag::resolution_tags;
  using recognition_data = std::vector<typename EngineTag::recognition_data_element>;

public: // Structors
  virtual ~ocr_engine() noexcept = default;

public: // Accessors
  ///
  /// Access to OCR engine name. This accessor is usefull for user engine selection.
  /// \return engine name
  ///
  virtual auto name() const -> name_type = 0;

public: // Modifiers
  ///
  /// Initialize the OCR engine with the given image, it only works on \p subarea if set.
  ///
  virtual auto initialize(pixel_plane_view_type image, std::optional<pixel_plane_view_type::area_type> subarea) -> void = 0;

  ///
  /// Recognize text on the image the engine was initialized.
  /// Depending on engine support; word, lines or paragraph information is returned.
  /// \return recognition result list (words, lines or paragraphs)
  ///
  virtual auto recognize() const -> recognition_data = 0;
};

///
/// Variant type for all supported OCR engines.
///
// clang-format off
using ocr_engine_uptr_variant_type = std::variant<
  std::monostate,
  std::unique_ptr<ocr_engine<ocr_engine_tag_plain>>,
  std::unique_ptr<ocr_engine<ocr_engine_tag_layout_analysis>>>;
// clang-format on

} // namespace bibstd::txt
