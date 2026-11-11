#pragma once

#include <string>

namespace arcstd::system
{

///
/// System clipboard.
///
struct clipboard final
{
  ///
  /// Replace the content of the clipboard with \p text. Blocks for a moment if another program holds the clipboard.
  /// \return true if successful, false otherwise
  ///
  [[nodiscard]] static auto set_text(const std::string& text) -> bool;
};

} // namespace arcstd::system
