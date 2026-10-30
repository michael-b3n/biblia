#pragma once

#include "bibstd/txt/ocr_engine.hpp"
#include "bibstd/util/language.hpp"

namespace bibstd::system
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
  [[nodiscard]] static auto create(util::language language) -> txt::ocr_engine_uptr_variant_type;
};

} // namespace bibstd::system
