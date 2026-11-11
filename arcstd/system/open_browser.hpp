#pragma once

#include <string>

namespace bibstd::system
{

///
/// Open URL in browser class for the windows OS.
///
struct open_browser final
{
  ///
  /// Open URL in the default web browser.
  /// \return true if successful, false otherwise
  ///
  [[nodiscard]] static auto open(const std::string& url) -> bool;
};

} // namespace bibstd::system
