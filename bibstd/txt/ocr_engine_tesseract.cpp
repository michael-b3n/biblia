#include "bibstd/txt/ocr_engine_tesseract.hpp"
#include "bibstd/math/coordinates.hpp"
#include "bibstd/system/filesystem.hpp"
#include "bibstd/util/const_map.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/numeric_cast.hpp"
#include "bibstd/util/visit_helper.hpp"

#include <leptonica/allheaders.h>
#include <leptonica/environ.h>
#include <leptonica/imageio.h>
#include <leptonica/pix_internal.h>
#include <tesseract/baseapi.h>
#include <tesseract/publictypes.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <system_error>
#include <vector>

namespace bibstd::txt
{
namespace
{

///
/// Forward the pixels struct as a leptonica PIX struct.
/// The pixels object is not copied and must live longer than the PIX object.
///
auto forward_as_pix(auto& data, const std::uint32_t width, const std::uint32_t height) -> Pix
{
  return Pix{
    /*l_uint32          */ .w = width,                                      // width in pixels
    /*l_uint32          */ .h = height,                                     // height in pixels
    /*l_uint32          */ .d = data::pixel::bits_per_pixel,                // depth in bits
    /*l_uint32          */ .spp = 4u,                                       // number of samples per pixel
    /*l_uint32          */ .wpl = width,                                    // 32-bit words/line
    /*l_uint32          */ .refcount = 1u,                                  // reference count (1 if no clones)
    /*l_int32           */ .xres = 0,                                       // image res (ppi) in x direction (use 0 if unknown)
    /*l_int32           */ .yres = 0,                                       // image res (ppi) in y direction (use 0 if unknown)
    /*l_int32           */ .informat = IFF_UNKNOWN,                         // input file format, IFF_*
    /*l_int32           */ .special = 0,                                    // special instructions for I/O, etc
    /*char              */ .text = nullptr,                                 // text string associated with pix
    /*struct PixColormap*/ .colormap = nullptr,                             // colormap (may be null)
    /*l_uint32          */ .data = reinterpret_cast<l_uint32*>(data.data()) // the image data
  };
}

///
/// Get the tesseract page iterator level from the resolution tag.
/// \return tesseract page iterator level ID
///
constexpr auto page_iterator_level(ocr_engine_tesseract::resolution_tags resolution_tag) -> tesseract::PageIteratorLevel
{
  using word_tag = ocr_engine_resolution_tag<ocr_engine_tesseract::word>;
  using line_tag = ocr_engine_resolution_tag<ocr_engine_tesseract::line>;
  using paragraph_tag = ocr_engine_resolution_tag<ocr_engine_tesseract::paragraph>;
  return util::visit_lambdas(
    resolution_tag,
    []([[maybe_unused]] word_tag) { return tesseract::RIL_WORD; },
    []([[maybe_unused]] line_tag) { return tesseract::RIL_TEXTLINE; },
    []([[maybe_unused]] paragraph_tag) { return tesseract::RIL_PARA; }
  );
}

///
/// Get the bounding box of the current iterator page object corresponding to the level (word, line, paragraph).
/// \return bounding box of text object if available, std::nullopt otherwise
///
auto get_bounding_box(const auto& ri, const auto level) -> std::optional<ocr_engine_tesseract::bounding_box_type>
{
  int left{};
  int top{};
  int right{};
  int bottom{};
  const auto found = ri->BoundingBox(level, &left, &top, &right, &bottom);
  if(found && left >= 0 && top >= 0 && right >= 1 && bottom >= 1 && right > left && bottom > top)
  {
    return ocr_engine_tesseract::bounding_box_type(math::coordinates{left, top}, math::coordinates{right, bottom});
  }
  else
  {
    return std::nullopt;
  }
}

///
/// Searches level by level, so the nearest folder is found first and nothing below it is opened.
/// \return the folder "tessdata" nearest below \p folders, std::nullopt if none is found opening \p max_folders
///
[[nodiscard]] auto nearest_tessdata_folder(const std::vector<std::filesystem::path>& folders, const std::size_t max_folders)
  -> std::optional<std::filesystem::path>
{
  auto error = std::error_code{};
  const auto subfolders = [&error](const std::filesystem::path& folder)
  {
    return std::filesystem::directory_iterator{folder, std::filesystem::directory_options::skip_permission_denied, error} |
           std::views::filter([&error](const auto& entry) { return entry.is_directory(error); }) |
           std::views::transform([](const auto& entry) { return entry.path(); });
  };
  const auto opened = folders | std::views::take(max_folders);
  if(std::ranges::empty(opened))
  {
    return std::nullopt;
  }
  const auto next = opened | std::views::transform(subfolders) | std::views::join | std::ranges::to<std::vector>();
  const auto found = std::ranges::find(next, std::filesystem::path{"tessdata"}, &std::filesystem::path::filename);
  return found != std::ranges::end(next) ? std::optional{*found}
                                         : nearest_tessdata_folder(next, max_folders - std::ranges::size(opened));
}

} // namespace

///
///
auto ocr_engine_tesseract::tessdata_folder_finder() -> std::optional<std::filesystem::path>
{
  const auto executable_folder_parent = system::filesystem::executable_folder().parent_path();
  const auto root = executable_folder_parent.parent_path();
  auto best_guess = executable_folder_parent / "share" / "tessdata";
  if(std::filesystem::exists(best_guess))
  {
    return best_guess;
  }
  // Folders opened at most, the ones above the executable may hold a whole drive
  static constexpr auto max_folders = std::size_t{1024};
  const auto result = nearest_tessdata_folder({executable_folder_parent}, max_folders);
  return result ? result : nearest_tessdata_folder({root}, max_folders);
}

///
///
ocr_engine_tesseract::ocr_engine_tesseract(const std::filesystem::path& tessdata_path, const util::language language)
  : tesseract_{new tesseract::TessBaseAPI()}
{
  if(!std::filesystem::exists(tessdata_path))
  {
    LOG_ERROR("tessdata path does not exist: \"{}\"", tessdata_path.generic_string());
    throw util::exception("non existent tessdata path");
  }
  auto tessdata_string = tessdata_path.generic_string();
  const auto lang = std::string{language_map.at(language)};
  tesseract_->Init(tessdata_string.data(), lang.c_str(), tesseract::OEM_LSTM_ONLY);
  tesseract_->SetVariable("lstm_choice_mode", "2"); // set lstm_choice_mode to alternative symbol choices per character
}

///
///
ocr_engine_tesseract::~ocr_engine_tesseract() noexcept
{
  tesseract_->End();
}

///
///
auto ocr_engine_tesseract::name() const -> name_type
{
  return name_type{"Tesseract"};
}

///
///
auto ocr_engine_tesseract::initialize(
  const pixel_plane_view_type image, const std::optional<pixel_plane_view_type::area_type> subarea
) -> void
{
  image_data_.clear();
  auto width = image.width();
  auto height = image.height();

  if(subarea)
  {
    using area_type = pixel_plane_view_type::area_type;
    const auto image_area = area_type{
      math::coordinates{0, 0},
      image.width(), image.height()
    };
    const auto clipped = math::overlap(*subarea, image_area);
    if(!clipped || math::empty(*clipped))
    {
      // The subarea lies outside the image, so there is nothing to recognize. Without an image
      // recognize() and layout_analysis() report an empty result instead of tesseract working
      // on a zero sized one.
      LOG_WARN("tesseract subarea lies outside the image: subarea={}", *subarea);
      tesseract_->Clear();
      return;
    }
    width = numeric_cast<decltype(width)>(math::size(clipped->horizontal_range()));
    height = numeric_cast<decltype(height)>(math::size(clipped->vertical_range()));
    image_data_.resize(image.data_view_size(*clipped));
    std::ranges::copy(image.data_view(*clipped), image_data_.begin());
  }
  else
  {
    image_data_.resize(image.size());
    std::ranges::copy(image, image_data_.begin());
  }
  // copy needed since pix requires to be non const
  auto pix = forward_as_pix(image_data_, width, height);
  tesseract_->SetImage(&pix);
  tesseract_->SetPageSegMode(tesseract::PSM_AUTO_OSD);
}

///
///
auto ocr_engine_tesseract::recognize() const -> recognition_data
{
  if(image_data_.empty())
  {
    return {};
  }
  if(const auto retval = tesseract_->Recognize(nullptr); retval != 0)
  {
    throw util::exception{std::format("tesseract recognition failure: code={}", retval)};
  }
  std::unique_ptr<tesseract::ResultIterator> ri(tesseract_->GetIterator());
  if(!ri)
  {
    throw util::exception{"tesseract result iterator creation failure"};
  }

  auto result = recognition_data{};

  auto current_paragraph = std::optional<paragraph>{};
  auto current_line = std::optional<line>{};
  auto current_word = std::optional<word>{};

  const auto get_txt = [&ri](const auto level) -> std::optional<std::string>
  {
    const std::unique_ptr<char[]> txt(ri->GetUTF8Text(level));
    if(txt)
    {
      return std::string{txt.get()};
    }
    return {};
  };

  const auto get_data = [&ri, &get_txt](auto tag)
  {
    using return_type = std::optional<typename decltype(tag)::underlying_type>;
    auto txt = get_txt(page_iterator_level(tag));
    auto bounding_box = get_bounding_box(ri, page_iterator_level(tag));
    return txt && bounding_box ? typename return_type::value_type{std::move(*txt), std::move(*bounding_box)} : return_type{};
  };

  do
  {
    auto word_data = get_data(tag<word>{});
    if(word_data)
    {
      auto line_data = get_data(tag<line>{});
      auto paragraph_data = get_data(tag<paragraph>{});

      result.emplace_back(
        recognition_data::value_type{
          .word_data = std::move(*word_data), .line_data = std::move(line_data), .paragraph_data = std::move(paragraph_data)
        }
      );
    }
  }
  while(ri->Next(page_iterator_level(tag<word>{})));
  return result;
}

///
///
auto ocr_engine_tesseract::layout_analysis() const -> std::vector<line_layout>
{
  static constexpr auto line_level = page_iterator_level(tag<line>{});
  auto result = std::vector<line_layout>{};
  if(image_data_.empty())
  {
    return result;
  }
  std::unique_ptr<tesseract::PageIterator> pi(tesseract_->AnalyseLayout(false));
  if(pi)
  {
    do
    {
      if(auto line_bounding_box = get_bounding_box(pi, line_level))
      {
        auto paragraph_bounding_box = get_bounding_box(pi, page_iterator_level(tag<paragraph>{}));
        result.emplace_back(
          line_layout{.line_bounding_box = *line_bounding_box, .paragraph_bounding_box = paragraph_bounding_box}
        );
      }
    }
    while(pi->Next(line_level));
  }
  return result;
}

} // namespace bibstd::txt
