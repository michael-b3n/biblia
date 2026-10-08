//
// One off capture that turns the images in `BIBSTD_TEST_OCR_DIR` into the OCR data the
// reference_ocr test replays. It runs the real tesseract engine and is therefore hidden from
// ctest: run it explicitly after adding or changing an image.
//
//   bibstd_test.exe "[.capture]"
//
// \see bibstd_test/res/ocr/README.md
//
#include "test_utils/ocr_capture_data.hpp"

#include <bibstd/data/pixel.hpp>
#include <bibstd/data/plane.hpp>
#include <bibstd/data/screen_types.hpp>
#include <bibstd/math/value_range.hpp>
#include <bibstd/txt/ocr_engine_tesseract.hpp>
#include <bibstd/util/language.hpp>

#include <catch2/catch_test_macros.hpp>

#include <leptonica/allheaders.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <format>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace bibstd::bible
{
namespace
{

///
/// Image formats leptonica reads for us, so that a screenshot can be dropped in as it is.
///
constexpr auto image_extensions = std::array{".png", ".bmp", ".jpg", ".jpeg", ".tif", ".tiff"};

///
/// Report progress of the capture. It is a tool rather than a check, so it says what it did.
///
auto report(const std::string& message) -> void
{
  std::cout << message << "\n";
}

///
/// Read an image of any supported format into a pixel plane.
/// \return Pixel plane with the image content, an empty plane if the image cannot be read
///
auto read_image(const std::filesystem::path& path) -> data::pixel_plane_type
{
  auto* raw = pixRead(path.string().c_str());
  if(raw == nullptr)
  {
    return data::pixel_plane_type{};
  }
  auto* rgb = pixConvertTo32(raw);
  pixDestroy(&raw);
  if(rgb == nullptr)
  {
    return data::pixel_plane_type{};
  }

  const auto width = static_cast<std::uint32_t>(pixGetWidth(rgb));
  const auto height = static_cast<std::uint32_t>(pixGetHeight(rgb));
  const auto words_per_line = static_cast<std::size_t>(pixGetWpl(rgb));
  const auto* const data = pixGetData(rgb);

  auto result = data::pixel_plane_type{width, height};
  for(auto y = std::uint32_t{0}; y < height; ++y)
  {
    const auto* const line = data + (static_cast<std::size_t>(y) * words_per_line);
    for(auto x = std::uint32_t{0}; x < width; ++x)
    {
      result.at((static_cast<std::size_t>(y) * width) + x) = data::pixel{
        .red = static_cast<std::uint8_t>(GET_DATA_BYTE(line, (4 * x) + COLOR_RED)),
        .green = static_cast<std::uint8_t>(GET_DATA_BYTE(line, (4 * x) + COLOR_GREEN)),
        .blue = static_cast<std::uint8_t>(GET_DATA_BYTE(line, (4 * x) + COLOR_BLUE)),
        .alpha = 255
      };
    }
  }
  pixDestroy(&rgb);
  return result;
}

///
/// \return Bounding box of an OCR element as capture box
///
auto to_capture_box(const auto& box) -> test_utils::capture_box
{
  return test_utils::capture_box{
    .x = static_cast<std::int32_t>(box.origin().x()),
    .y = static_cast<std::int32_t>(box.origin().y()),
    .width = static_cast<std::int32_t>(math::size(box.horizontal_range())),
    .height = static_cast<std::int32_t>(math::size(box.vertical_range()))
  };
}

///
/// \return Text and bounding box of an OCR element as capture text
///
auto to_capture_text(const auto& element) -> test_utils::capture_text
{
  return test_utils::capture_text{.text = element.text, .box = to_capture_box(element.bounding_box)};
}

///
/// \return Every image of the folder, sorted, empty when the folder holds none
///
auto images_in(const std::filesystem::path& folder) -> std::vector<std::filesystem::path>
{
  auto result = std::vector<std::filesystem::path>{};
  for(const auto& entry : std::filesystem::directory_iterator{folder})
  {
    // Screenshots arrive with whatever case the tool that made them chose.
    auto extension = entry.path().extension().string();
    std::ranges::transform(extension, extension.begin(), [](const auto c) { return std::tolower(c); });
    if(std::ranges::contains(image_extensions, extension))
    {
      result.push_back(entry.path());
    }
  }
  std::ranges::sort(result);
  return result;
}

///
/// The capture file is named after the image stem, so two images sharing one would overwrite each
/// other silently.
/// \return Stem shared by two images, std::nullopt when every stem is unique
///
auto duplicate_stem(const std::vector<std::filesystem::path>& images) -> std::optional<std::string>
{
  auto stems = std::vector<std::string>{};
  std::ranges::transform(images, std::back_inserter(stems), [](const auto& path) { return path.stem().string(); });
  std::ranges::sort(stems);
  const auto it = std::ranges::adjacent_find(stems);
  return it != std::ranges::cend(stems) ? std::optional{*it} : std::nullopt;
}

///
/// \return Everything the engine recognized on the image it was initialized with
///
auto capture_of(const std::string& id, const txt::ocr_engine_tesseract& engine, const data::pixel_plane_type& image)
  -> test_utils::capture_data
{
  auto result = test_utils::capture_data{.id = id, .width = image.width(), .height = image.height()};
  for(const auto& layout : engine.layout_analysis())
  {
    result.layouts.emplace_back(
      test_utils::capture_layout{
        .line = to_capture_box(layout.line_bounding_box),
        .paragraph =
          layout.paragraph_bounding_box ? std::optional{to_capture_box(*layout.paragraph_bounding_box)} : std::nullopt
      }
    );
  }
  for(const auto& element : engine.recognize())
  {
    result.words.emplace_back(
      test_utils::capture_word{
        .word = to_capture_text(element.word_data),
        .line = element.line_data ? std::optional{to_capture_text(*element.line_data)} : std::nullopt,
        .paragraph = element.paragraph_data ? std::optional{to_capture_text(*element.paragraph_data)} : std::nullopt
      }
    );
  }
  return result;
}

///
/// A capture whose image is gone would keep being replayed by the test, so drop the orphans.
///
auto remove_orphan_captures(const std::filesystem::path& folder, const std::vector<std::filesystem::path>& images) -> void
{
  for(const auto& entry : std::filesystem::directory_iterator{folder})
  {
    if(entry.path().extension() != std::filesystem::path{".ocr"})
    {
      continue;
    }
    const auto has_image = std::ranges::any_of(images, [&](const auto& p) { return p.stem() == entry.path().stem(); });
    if(!has_image)
    {
      report(std::format("removed orphan capture: {}", entry.path().filename().string()));
      std::filesystem::remove(entry.path());
    }
  }
}

} // namespace

TEST_CASE("capture ocr data of the images in the ocr folder", "[.capture]")
{
  const auto folder = std::filesystem::path{BIBSTD_TEST_OCR_DIR};
  REQUIRE(std::filesystem::is_directory(folder));

  const auto images = images_in(folder);
  REQUIRE_FALSE(images.empty());
  if(const auto duplicate = duplicate_stem(images))
  {
    FAIL(std::format("two images share the stem \"{}\", rename one of them", *duplicate));
  }

  const auto tessdata = txt::ocr_engine_tesseract::tessdata_folder_finder();
  if(!tessdata)
  {
    FAIL("no tessdata folder found, the capture needs the traineddata the application ships");
  }
  auto engine = txt::ocr_engine_tesseract{*tessdata, util::language::german};

  for(const auto& path : images)
  {
    const auto image = read_image(path);
    if(image.empty())
    {
      report(std::format("skipped, image cannot be read: {}", path.filename().string()));
      continue;
    }
    engine.initialize(data::pixel_plane_view_type{image}, std::nullopt);
    const auto data = capture_of(path.stem().string(), engine, image);

    const auto capture_path = std::filesystem::path{path}.replace_extension(".ocr");
    CHECK(test_utils::write_capture(capture_path, data));
    report(
      std::format(
        "{}: {} words, {} layout lines, image {}x{}",
        capture_path.filename().string(),
        data.words.size(),
        data.layouts.size(),
        data.width,
        data.height
      )
    );
  }

  remove_orphan_captures(folder, images);
}

} // namespace bibstd::bible
