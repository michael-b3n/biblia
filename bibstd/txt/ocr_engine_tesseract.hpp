#pragma once

#include "bibstd/txt/ocr_engine.hpp"
#include "bibstd/util/const_map.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/language.hpp"

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

// Forward declarations
namespace tesseract
{
class TessBaseAPI;
} // namespace tesseract
namespace bibstd::data
{
class pix;
} // namespace bibstd::data

namespace bibstd::txt
{

///
/// OCR tesseract engine. This class uses OCR on loaded bitmaps to recognize text and bounding boxes.
///
class ocr_engine_tesseract final : public ocr_engine<ocr_engine_tag_layout_analysis>
{
  // Constants
  static constexpr auto language_map = util::make_const_bimap<util::language, std::string_view>({
    { util::language::german, "deu"},
    {util::language::english, "eng"},
    // ...
  });
  static_assert(language_map.size() == util::enum_count<util::language>());

  // Variables
  const std::unique_ptr<tesseract::TessBaseAPI> tesseract_;
  std::vector<pixel_plane_view_type::value_type> image_data_;

public: // Static
  ///
  /// Find the tessdata folder: `share/tessdata` next to the folder of the executable as installed, else the
  /// nearest folder "tessdata" below the parent of the executable folder, else below the parent of that.
  /// \return optional path to tessdata folder, std::nullopt if not found
  ///
  [[nodiscard]] static auto tessdata_folder_finder() -> std::optional<std::filesystem::path>;

public: // Structors
  ocr_engine_tesseract(const std::filesystem::path& tessdata_path, util::language language);
  ~ocr_engine_tesseract() noexcept override;

public: // Overrides
  ///
  /// \see ocr_engine::name
  ///
  auto name() const -> name_type override;

  ///
  /// \see ocr_engine::initialize
  ///
  auto initialize(pixel_plane_view_type image, std::optional<pixel_plane_view_type::area_type> subarea) -> void override;

  ///
  /// \see ocr_engine::recognize
  ///
  auto recognize() const -> recognition_data override;

  ///
  /// \see ocr_engine::layout_analysis
  ///
  auto layout_analysis() const -> std::vector<line_layout> override;
};

} // namespace bibstd::txt
