#pragma once

#include "arcstd/util/language.hpp"
#include "bibocr/txt/ocr_engine.hpp"

namespace bibocr::system
{

///
/// System OCR interface. Provides platform-specific OCR engine interface and capability queries.
///
struct ocr final
{
  // Static
  ///
  /// Create the system OCR engine for \p language.
  /// \return the engine, std::monostate if the system has none or it fails
  ///
  [[nodiscard]] static auto create(arcstd::util::language language) -> bibocr::txt::ocr_engine_uptr_variant_type;
};

} // namespace bibocr::system
