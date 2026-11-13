#pragma once

#include "bibocr/txt/ocr_engine.hpp"
#include "bibocr/util/language.hpp"

#include <arcstd/util/const_map.hpp>
#include <arcstd/util/enum.hpp>

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace tesseract
{
class TessBaseAPI;
} // namespace tesseract

namespace bibocr::data
{
class pix;
} // namespace bibocr::data

namespace bibocr::txt
{

///
/// OCR tesseract engine. This class uses OCR on loaded bitmaps to recognize text and bounding boxes.
///
class ocr_engine_tesseract final : public ocr_engine<ocr_engine_tag_layout_analysis>
{
  // Constants
  static constexpr auto language_map = arcstd::util::make_const_bimap<bibocr::util::language, std::string_view>({
    { bibocr::util::language::german, "deu"},
    {bibocr::util::language::english, "eng"},
    // ...
  });
  static_assert(language_map.size() == arcstd::util::enum_count<bibocr::util::language>());

  // Variables
  const std::unique_ptr<tesseract::TessBaseAPI> tesseract_;
  std::vector<bibocr::data::pixel_plane_view_type::value_type> image_data_;

public: // Static
  ///
  /// \return the tessdata folder the app ships: `share/tessdata` next to the folder of the executable
  ///
  [[nodiscard]] static auto shipped_tessdata_folder() -> std::filesystem::path;

public: // Structors
  ocr_engine_tesseract(const std::filesystem::path& tessdata_path, bibocr::util::language language);
  ~ocr_engine_tesseract() noexcept override;

public: // Overrides
  ///
  /// \see ocr_engine::name
  ///
  [[nodiscard]] auto name() const -> name_type override;

  ///
  /// \see ocr_engine::initialize
  ///
  auto initialize(
    bibocr::data::pixel_plane_view_type image, std::optional<bibocr::data::pixel_plane_view_type::area_type> subarea
  ) -> void override;

  ///
  /// \see ocr_engine::recognize
  ///
  [[nodiscard]] auto recognize() const -> recognition_data override;

  ///
  /// \see ocr_engine::layout_analysis
  ///
  [[nodiscard]] auto layout_analysis() const -> std::vector<line_layout> override;
};

} // namespace bibocr::txt
