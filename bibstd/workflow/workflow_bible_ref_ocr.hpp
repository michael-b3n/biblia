#pragma once

#include "bibstd/bible/reference_ocr.hpp"
#include "bibstd/bible/reference_range.hpp"
#include "bibstd/data/screen_types.hpp"
#include "bibstd/framework/process_params.hpp"
#include "bibstd/util/language.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_scripture.hpp"
#include "bibstd/workflow/workflow_settings_base.hpp"

#include <expected>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace bibstd::workflow
{

///
/// Settings corresponding to workflow bible reference ocr.
///
struct workflow_bible_ref_ocr_settings final : public workflow_settings_base
{
  // Typedefs
  ///
  /// Algorithm recognizing the text around the position, \see bible::reference_ocr::algorithm_type.
  ///
  enum class ocr_recognition_algorithm
  {
    // Reads the whole image, needs no layout analysis and so works with every engine.
    line_recognition,
    // Reads only the lines around the position, needs an engine with layout analysis, e.g. tesseract.
    paragraph_recognition,
  };

  // Structors
  workflow_bible_ref_ocr_settings(std::shared_ptr<workflow_settings> workflow_settings);

  // Variables
  const setting_type<std::optional<std::filesystem::path>> tessdata_path;
  const setting_type<std::optional<std::string>> character_recognition_ocr_engine;
  const setting_type<std::optional<std::string>> layout_recognition_ocr_engine;
  const setting_type<ocr_recognition_algorithm> recognition_algorithm;
  const setting_type<util::language> language;
};

///
/// Bible reference ocr: this workflow searches for bible references on a screen area using OCR around the cursor position.
///
class workflow_bible_ref_ocr final : public workflow_base<workflow_bible_ref_ocr_settings>
{
  // Typedefs
  struct params_t final
  {
    data::pixel_plane_view_type image;
    data::screen_coordinates_type position;
  };

  struct result_t final
  {
    std::vector<bible::reference_range> reference_ranges;
    std::optional<data::screen_rect_type> reference_bounding_box;
  };

  struct find_references_result_t final
  {
    std::vector<bible::reference_range> ranges;
    std::optional<data::screen_rect_type> bounding_box;
  };

  struct settings_t final
  {
    std::string character_recognition_ocr_engine;
    std::optional<std::string> layout_recognition_ocr_engine;
    workflow_bible_ref_ocr_settings::ocr_recognition_algorithm recognition_algorithm;
    util::language language;
    workflow_scripture::versification_wrapper_type versification;
  };

  using position_data_result_type =
    std::expected<bible::reference_ocr::reference_position_data, bible::reference_ocr::unexpected_ocr_result>;

  // Variables
  mutable std::mutex mtx_;
  const std::shared_ptr<workflow_scripture> workflow_scripture_;
  bible::reference_ocr::ocr_engine_list_type ocr_engines_;

public: // Typedefs
  using params = framework::process_params<params_t>;
  using result = framework::process_result<result_t>;
  using ocr_recognition_algorithm = workflow_bible_ref_ocr_settings::ocr_recognition_algorithm;

public: // Structors
  ///
  /// Constructor for workflow_bible_ref_ocr.
  /// If tesseract is used for OCR, a valid tessdata path is required:
  /// \see txt::ocr_engine_tesseract::tessdata_folder_finder.
  ///
  workflow_bible_ref_ocr(
    std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_scripture> workflow_scripture
  );
  ~workflow_bible_ref_ocr() noexcept override;

public: // Modifiers
  ///
  /// Search bible references on an image using OCR around the specified position.
  /// \return list of found bible reference ranges, or an unexpected result in case of failure
  ///
  [[nodiscard]] auto find(const params& params) -> result;

private: // Implementation
  auto init() -> void;
  auto load_ocr_engines() -> void;
  auto limit_settings_to_loaded_engines() -> void;
  [[nodiscard]] auto find_references(
    const auto& params, const settings_t& settings, const position_data_result_type& position_data
  ) -> framework::process_result<find_references_result_t>;
};

} // namespace bibstd::workflow
