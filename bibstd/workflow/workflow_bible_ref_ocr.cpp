#include "bibstd/workflow/workflow_bible_ref_ocr.hpp"
#include "bibstd/bible/reference_ocr.hpp"
#include "bibstd/bible/reference_parser.hpp"
#include "bibstd/system/locale.hpp"
#include "bibstd/system/ocr.hpp"
#include "bibstd/txt/ocr_engine.hpp"
#include "bibstd/txt/ocr_engine_tesseract.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/format.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/ranges.hpp"
#include "bibstd/util/visit_helper.hpp"
#include "bibstd/workflow/workflow_scripture.hpp"
#include "bibstd/workflow/workflow_settings.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace bibstd::workflow
{
namespace
{

///
/// Compute the bounding box of the recognized reference within the image. The bounding box
/// surrounds the bounding boxes of all characters the reference was parsed from.
/// \return bounding box of the reference, or std::nullopt if no bounding box could be determined
///
[[nodiscard]] auto reference_bounding_box(
  const bible::reference_ocr::reference_position_data& position_data,
  const bible::reference_parser::index_range_type& index_range
) -> std::optional<util::screen_rect_type>
{
  decltype(auto) boxes = position_data.character_bounding_boxes;
  const auto begin = std::ranges::next(std::ranges::cbegin(boxes), std::min(index_range.begin, boxes.size()));
  const auto end = std::ranges::next(std::ranges::cbegin(boxes), std::min(index_range.end, boxes.size()));

  auto result = std::optional<util::screen_rect_type>{};
  std::ranges::for_each(
    std::ranges::subrange(begin, end) | std::views::filter([](const auto& box) { return box.has_value(); }),
    [&](const auto& box) { result = result ? math::surrounding_rect(*result, *box) : *box; }
  );
  return result;
}

///
/// Blank the text outside \p index_range, the indices of the characters stay the same.
/// \return text of \p index_range surrounded by spaces
///
[[nodiscard]] auto text_within(const std::string_view text, const math::value_range<std::size_t> index_range) -> std::string
{
  return util::ranges::index_view_to(text.size()) |
         std::views::transform([&](const auto index) { return math::contains(index_range, index) ? text.at(index) : ' '; }) |
         std::ranges::to<std::string>();
}

///
/// \return name of \p engine
///
[[nodiscard]] auto name_of(const txt::ocr_engine_uptr_variant_type& engine) -> std::string
{
  return util::visit_lambdas(
    engine,
    []([[maybe_unused]] const std::monostate&) { return std::string{}; },
    [](const auto& e) { return std::string{e->name()}; }
  );
}

///
/// \return true if \p engine analyses the layout, which the paragraph recognition needs
///
[[nodiscard]] auto has_layout_analysis(const txt::ocr_engine_uptr_variant_type& engine) -> bool
{
  return std::holds_alternative<txt::ocr_engine<txt::ocr_engine_tag_layout_analysis>::uptr_type>(engine);
}

///
/// Algorithm an engine reads best with. Tesseract takes many seconds for a whole window, reading only the lines its
/// layout analysis finds around the position takes about one and a half. The system engine has no layout analysis and
/// reads a whole window in a fraction of a second.
/// \return preferred algorithm of \p engine
///
[[nodiscard]] auto preferred_algorithm(const txt::ocr_engine_uptr_variant_type& engine)
  -> workflow_bible_ref_ocr::ocr_recognition_algorithm
{
  using enum workflow_bible_ref_ocr::ocr_recognition_algorithm;
  return has_layout_analysis(engine) ? paragraph_recognition : line_recognition;
}

///
/// \return reference ocr algorithm of the setting value \p algorithm
///
[[nodiscard]] auto algorithm_of(const workflow_bible_ref_ocr::ocr_recognition_algorithm algorithm)
  -> bible::reference_ocr::algorithm_type
{
  using enum workflow_bible_ref_ocr::ocr_recognition_algorithm;
  switch(algorithm)
  {
  case line_recognition: return bible::reference_ocr::line_recognition{};
  case paragraph_recognition: return bible::reference_ocr::paragraph_recognition{};
  }
  std::unreachable();
}

///
/// Limit \p setting to the \p available values. The validator resets a value that is not available, an optional value to
/// nothing and any other value to the first available one.
///
template<framework::underlying_setting_type T>
auto limit_to(
  const workflow_settings::setting_non_owning_ptr_type<T>& setting,
  const std::vector<typename framework::setting_validator_list<T>::plain_underlying_type>& available
) -> void
{
  // Only an empty list of a non optional setting is refused, the lists given here always hold line recognition.
  std::ignore = std::get<typename framework::setting_validator_list<T>::sptr_type>(setting->validator)->available(available);
}

} // namespace

///
///
workflow_bible_ref_ocr_settings::workflow_bible_ref_ocr_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : framework::settings_base{std::move(workflow_settings)}
  , tessdata_path{workflow_settings_->create_setting("ocr.tessdata_path", txt::ocr_engine_tesseract::tessdata_folder_finder())}
  , character_recognition_ocr_engine{workflow_settings_->create_setting(
      "ocr.character_recognition_ocr_engine",
      setting_value_t<decltype(character_recognition_ocr_engine)>{},
      std::make_shared<framework::setting_validator_list<setting_value_t<decltype(character_recognition_ocr_engine)>>>()
    )}
  , layout_recognition_ocr_engine{workflow_settings_->create_setting(
      "ocr.layout_recognition_ocr_engine",
      setting_value_t<decltype(layout_recognition_ocr_engine)>{},
      std::make_shared<framework::setting_validator_list<setting_value_t<decltype(layout_recognition_ocr_engine)>>>()
    )}
  , recognition_algorithm{workflow_settings_->create_setting(
      "ocr.recognition_algorithm", ocr_recognition_algorithm::line_recognition
    )}
  , language{workflow_settings_->create_setting("ocr.language", system::locale::preferred_language())}
{
}

///
///
workflow_bible_ref_ocr::workflow_bible_ref_ocr(
  std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_scripture> workflow_scripture
)
  : workflow_base{std::move(workflow_settings)}
  , workflow_scripture_{std::move(workflow_scripture)}
{
  init();
}

///
///
workflow_bible_ref_ocr::~workflow_bible_ref_ocr() noexcept = default;

///
///
auto workflow_bible_ref_ocr::find(const params& params) -> result
{
  try
  {
    const auto lock = std::scoped_lock{mtx_};

    const auto character_recognition_ocr_engine = settings().character_recognition_ocr_engine->value();
    if(!character_recognition_ocr_engine.has_value() || ocr_engines_.empty())
    {
      LOG_WARN("failed to start bible reference ocr search: no ocr engines initialized");
      return return_failure;
    }
    const auto local_settings = settings_t{
      .character_recognition_ocr_engine = *character_recognition_ocr_engine,
      .layout_recognition_ocr_engine = settings().layout_recognition_ocr_engine->value(),
      .recognition_algorithm = settings().recognition_algorithm->value(),
      .language = settings().language->value(),
      .versification = workflow_scripture_->versification_or_fallback({{}})
    };
    LOG_DEBUG(
      "find references: image=[width={}, height={}], position=[{}]",
      params->image.width(),
      params->image.height(),
      params->position
    );

    const auto construct_result = [](const find_references_result_t& found) -> result
    {
      auto retval = result{};
      if(!found.ranges.empty())
      {
        LOG_INFO("reference search finished: references=[{}]", util::format::join(found.ranges, ", "));
        auto ranges = found.ranges;
        std::ranges::sort(ranges, [](const auto& a, const auto& b) { return a.begin() < b.begin(); });
        retval = result::value_type{.reference_ranges = std::move(ranges), .reference_bounding_box = found.bounding_box};
      }
      else
      {
        LOG_DEBUG("reference search finished: no references found");
      }
      return retval;
    };
    const auto found = [](const auto& references) { return references && !references->ranges.empty(); };

    const auto engine_names = bible::reference_ocr::engine_names{
      .character_recognition = local_settings.character_recognition_ocr_engine,
      .layout_recognition = local_settings.layout_recognition_ocr_engine
    };
    const auto position_data = bible::reference_ocr::run(
      ocr_engines_, engine_names, params->image, params->position, algorithm_of(local_settings.recognition_algorithm)
    );
    const auto references = find_references(params, local_settings, position_data);

    // OCR is not perfect, small text the engine dropped is often read once it is enlarged. No single scale reads every
    // line, e.g. "Jes" of "(Jes 48,1" is read at twice the size and "Gal" of "(Joh 7,19; Gal 6,13)" at one and a half.
    // A found reference is kept, find_references parses only consecutive characters so dropped text does not change it.
    static constexpr auto enlargement_scales = std::array{1.5, 2.0};
    const auto enlarged_lines_references = [&](const double scale)
    {
      const auto enlarged_position_data = bible::reference_ocr::run(
        ocr_engines_,
        engine_names,
        params->image,
        params->position,
        bible::reference_ocr::enlarged_lines_recognition{.earlier_recognition = *position_data, .scale = scale}
      );
      return find_references(params, local_settings, enlarged_position_data);
    };
    const auto read_enlarged_while_none_found = [&](auto references_so_far, const double scale)
    { return found(references_so_far) || !position_data ? references_so_far : enlarged_lines_references(scale); };
    return std::ranges::fold_left(enlargement_scales, references, read_enlarged_while_none_found).and_then(construct_result);
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    return return_failure;
  }
}

///
///
auto workflow_bible_ref_ocr::init() -> void
{
  const auto lock = std::scoped_lock{mtx_};
  load_ocr_engines();
  limit_settings_to_loaded_engines();
}

///
///
auto workflow_bible_ref_ocr::load_ocr_engines() -> void
{
  auto ocr_engine_system = system::ocr::create(settings().language->value());
  if(!std::holds_alternative<std::monostate>(ocr_engine_system))
  {
    ocr_engines_.emplace_back(std::move(ocr_engine_system));
  }
  if(!settings().tessdata_path->value() || !std::filesystem::exists(*settings().tessdata_path->value()))
  {
    if(const auto found_tessdata_folder = txt::ocr_engine_tesseract::tessdata_folder_finder())
    {
      LOG_WARN("tessdata path setting invalid: used_alternative_folder=\"{}\"", found_tessdata_folder->generic_string());
      settings().tessdata_path->value(found_tessdata_folder);
    }
  }
  const auto path = settings().tessdata_path->value();
  if(path && std::filesystem::exists(*path))
  {
    ocr_engines_.emplace_back(std::make_unique<txt::ocr_engine_tesseract>(*path, settings().language->value()));
  }
}

///
///
auto workflow_bible_ref_ocr::limit_settings_to_loaded_engines() -> void
{
  const auto character_recognition_engines = ocr_engines_ | std::views::transform(name_of) | std::ranges::to<std::vector>();
  const auto layout_recognition_engines =
    ocr_engines_ | std::views::filter(has_layout_analysis) | std::views::transform(name_of) | std::ranges::to<std::vector>();
  const auto recognition_algorithms =
    layout_recognition_engines.empty()
      ? std::vector{ocr_recognition_algorithm::line_recognition}
      : std::vector{ocr_recognition_algorithm::line_recognition, ocr_recognition_algorithm::paragraph_recognition};
  // A chosen engine that is not loaded is reset to nothing, paragraph recognition without tesseract to line recognition.
  limit_to(settings().character_recognition_ocr_engine, character_recognition_engines);
  limit_to(settings().layout_recognition_ocr_engine, layout_recognition_engines);
  limit_to(settings().recognition_algorithm, recognition_algorithms);

  // Without a chosen engine the first loaded one is taken, the system engine first, with the algorithm it reads best with.
  if(!settings().character_recognition_ocr_engine->value() && !ocr_engines_.empty())
  {
    settings().character_recognition_ocr_engine->value(name_of(ocr_engines_.front()));
    settings().recognition_algorithm->value(preferred_algorithm(ocr_engines_.front()));
  }
  if(!settings().layout_recognition_ocr_engine->value() && !layout_recognition_engines.empty())
  {
    settings().layout_recognition_ocr_engine->value(layout_recognition_engines.front());
  }
}

///
///
auto workflow_bible_ref_ocr::find_references(
  const auto& params, const settings_t& settings, const position_data_result_type& position_data
) -> framework::process_result<find_references_result_t>
{
  if(position_data)
  {
    decltype(auto) versification = settings.versification.get();
    const auto parse = [&](const std::string_view text)
    { return bible::reference_parser::parse(text, position_data->cursor_character_index, settings.language, versification); };
    auto parse_result = parse(position_data->text);
    const auto consecutive =
      bible::reference_ocr::consecutive_characters(*position_data, params->position, parse_result.index_range_origin);
    if(consecutive != math::value_range<std::size_t>{0u, position_data->text.size()})
    {
      LOG_DEBUG(
        "text dropped next to the reference, parsing the consecutive characters: range=[{}, {})",
        consecutive.begin,
        consecutive.end
      );
      parse_result = parse(text_within(position_data->text, consecutive));
    }
    return find_references_result_t{
      .ranges = std::move(parse_result.ranges),
      .bounding_box = reference_bounding_box(*position_data, parse_result.index_range_origin)
    };
  }
  else
  {
    return return_failure;
  }
}

} // namespace bibstd::workflow
