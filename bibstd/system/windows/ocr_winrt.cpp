#include "bibstd/math/coordinates.hpp"
#include "bibstd/meta/lossless_conversion.hpp"
#include "bibstd/system/ocr.hpp"
#include "bibstd/system/windows/language_tag.hpp"
#include "bibstd/txt/ocr_engine.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/language.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/numeric_cast.hpp"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <ranges>
#include <variant>

namespace bibstd::system
{
namespace
{

namespace winrt_ocr = winrt::Windows::Media::Ocr;
namespace winrt_imaging = winrt::Windows::Graphics::Imaging;
namespace winrt_globalization = winrt::Windows::Globalization;

///
/// Map util::language to Windows BCP-47 language tag.
/// \return BCP-47 language tag corresponding to the given language
///
auto to_language_tag(const util::language language) -> winrt::hstring
{
  return winrt::hstring{language_wtag_map.at(language)};
}

///
/// Windows OCR engine implementation using Windows.Media.Ocr WinRT API.
/// \see base class ocr::engine
///
class ocr_engine_windows final : public txt::ocr_engine<txt::ocr_engine_tag_plain>
{
  // Variables
  winrt_ocr::OcrEngine engine_{nullptr};
  std::optional<winrt_imaging::SoftwareBitmap> bitmap_;
  mutable recognition_data recognition_data_;

public: // Structors
  explicit ocr_engine_windows(util::language language);

public: // Accessors
  ///
  /// Check if the OCR engine was successfully created and is available for use.
  /// \return true if the OCR engine is initialized and ready, false otherwise
  ///
  auto initialized() const -> bool;

public: // Overrides$
  ///
  /// \see ocr::ocr_engine::name
  ///
  auto name() const -> name_type override;

  ///
  /// \see ocr::engine::set_image
  ///
  auto initialize(pixel_plane_view_type image, std::optional<pixel_plane_view_type::area_type> subarea) -> void override;

  ///
  /// \see ocr::engine::recognize
  ///
  auto recognize() const -> recognition_data override;

private: // Implementation
  auto create_bitmap(pixel_plane_view_type image, std::optional<pixel_plane_view_type::area_type> subarea) -> void;
  static auto compute_line_bounding_box(const std::vector<word>& words) -> bounding_box_type;
};

///
///
ocr_engine_windows::ocr_engine_windows(const util::language language)
{
  const auto tag = to_language_tag(language);
  const auto lang = winrt_globalization::Language(tag);
  if(
    winrt_ocr::OcrEngine::IsLanguageSupported(lang) &&
    util::language_direction_map.at(language) == util::language_direction::ltr
  )
  {
    engine_ = winrt_ocr::OcrEngine::TryCreateFromLanguage(lang);
  }
}

///
///
auto ocr_engine_windows::initialized() const -> bool
{
  return static_cast<bool>(engine_);
}

///
///
auto ocr_engine_windows::name() const -> name_type
{
  return name_type{"System (Windows)"};
}

///
///
auto ocr_engine_windows::initialize(
  const pixel_plane_view_type image, const std::optional<pixel_plane_view_type::area_type> subarea
) -> void
{
  create_bitmap(image, subarea);
  recognition_data_.clear();
}

///
///
auto ocr_engine_windows::recognize() const -> recognition_data
{
  if(!engine_)
  {
    throw util::exception{"Windows OCR engine is not initialized"};
  }
  if(!bitmap_)
  {
    LOG_WARN("Windows OCR: bitmap empty");
    return {};
  }
  try
  {
    const auto result = engine_.RecognizeAsync(*bitmap_).get();
    recognition_data_.clear();

    // The offset for converting Windows OCR coordinates back to image coordinates
    const auto to_word_data = [](const auto& w)
    {
      const auto rect = w.BoundingRect();
      const auto origin = math::coordinates{numeric_cast<std::int32_t>(rect.X), numeric_cast<std::int32_t>(rect.Y)};
      const auto corner = math::coordinates{
        origin.x() + numeric_cast<std::int32_t>(rect.Width), origin.y() + numeric_cast<std::int32_t>(rect.Height)
      };
      return word{winrt::to_string(w.Text()), bounding_box_type(origin, corner)};
    };

    for(const auto& l : result.Lines())
    {
      auto words_data = l.Words() | std::views::transform(to_word_data) | std::ranges::to<std::vector>();
      auto combined_bounding_box = compute_line_bounding_box(words_data);
      auto line_data = line{.text = winrt::to_string(l.Text()), .bounding_box = combined_bounding_box};
      recognition_data_.reserve(words_data.size());
      for(auto& word_data : words_data)
      {
        recognition_data_.emplace_back(recognition_data::value_type{.word_data = std::move(word_data), .line_data = line_data});
      }
    }

    return recognition_data_;
  }
  catch(const winrt::hresult_error& e)
  {
    LOG_ERROR("Windows OCR recognition failed: {}", winrt::to_string(e.message()));
    return {};
  }
  catch(...)
  {
    LOG_ERROR("unknown exception in Windows OCR recognition: {}", util::exception_report());
    return {};
  }
}

///
///
auto ocr_engine_windows::create_bitmap(
  const pixel_plane_view_type image, const std::optional<pixel_plane_view_type::area_type> subarea
) -> void
{
  const auto [width, height] = [&]
  {
    using area_type = decltype(subarea)::value_type;
    using size_type = std::uint64_t;
    if(subarea)
    {
      const auto overlap = math::overlap(*subarea, area_type(math::coordinates{0, 0}, image.width(), image.height()));
      return overlap ? std::pair{math::size(overlap->horizontal_range()), math::size(overlap->vertical_range())}
                     : std::pair{size_type{0}, size_type{0}};
    }
    else
    {
      static_assert(meta::is_lossless_conversion_v<decltype(image.width()), size_type>);
      static_assert(meta::is_lossless_conversion_v<decltype(image.height()), size_type>);
      return std::pair{static_cast<size_type>(image.width()), static_cast<size_type>(image.height())};
    }
  }();

  // Create SoftwareBitmap with Bgra8 format (alpha ignored since screen captures have no alpha)
  bitmap_ = winrt_imaging::SoftwareBitmap(
    winrt_imaging::BitmapPixelFormat::Bgra8,
    numeric_cast<int>(width),
    numeric_cast<int>(height),
    winrt_imaging::BitmapAlphaMode::Ignore
  );

  // Copy pixel data. The pixel_plane is stored top-down (compatible with Tesseract/Leptonica),
  // and SoftwareBitmap also expects top-down, so no row flipping is needed.

  auto buffer = bitmap_.value().LockBuffer(winrt_imaging::BitmapBufferAccessMode::Write);
  auto reference = buffer.CreateReference();

  auto [dst_data, dst_capacity] = [&]
  {
    std::uint8_t* ptr = nullptr;
    std::uint32_t cap = 0;
    auto byte_access = reference.as<winrt::impl::IMemoryBufferByteAccess>();
    byte_access->GetBuffer(&ptr, &cap);
    return std::pair{ptr, cap};
  }();

  const auto for_each_pixel = [&](const auto& p)
  {
    const auto& [i, pixel] = p;
    // pixel is RGBA, SoftwareBitmap expects BGRA. Alpha is set to 255 (opaque)
    // because screen captures do not set the alpha channel.
    dst_data[(i * 4) + 0] = pixel.blue;
    dst_data[(i * 4) + 1] = pixel.green;
    dst_data[(i * 4) + 2] = pixel.red;
    dst_data[(i * 4) + 3] = 255;
  };

  if(subarea)
  {
    std::ranges::for_each(image.data_view(*subarea) | std::views::enumerate, for_each_pixel);
  }
  else
  {
    std::ranges::for_each(image | std::views::enumerate, for_each_pixel);
  }
  reference.Close();
  buffer.Close();
}

///
///
auto ocr_engine_windows::compute_line_bounding_box(const std::vector<word>& words) -> bounding_box_type
{
  auto rect = bounding_box_type(math::coordinates{0, 0}, 0u, 0u);
  std::ranges::for_each(words, [&rect](const auto& word) { rect = math::surrounding_rect(rect, word.bounding_box); });
  return rect;
}

} // namespace

///
///
auto ocr::create(const util::language language) -> txt::ocr_engine_uptr_variant_type
{
  using return_type = std::unique_ptr<txt::ocr_engine<txt::ocr_engine_tag_plain>>;
  return_type engine = std::make_unique<ocr_engine_windows>(language);
  if(!static_cast<ocr_engine_windows&>(*engine).initialized())
  {
    LOG_ERROR("failed to initialize Windows OCR engine");
    return std::monostate{};
  }
  return engine;
}

} // namespace bibstd::system
