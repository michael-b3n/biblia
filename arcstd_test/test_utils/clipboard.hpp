#pragma once

#include <arcstd/system/clipboard.hpp>
#include <arcstd/util/exception.hpp>

#include <optional>
#include <string>
#include <tuple>

namespace bibstd::test_utils
{

///
/// Another program may hold the clipboard for a moment, so it is tried several times.
/// \return the text of the clipboard as UTF-8, empty if there is none, std::nullopt if the clipboard could not be opened
///
[[nodiscard]] auto clipboard_text() -> std::optional<std::string>;

///
/// Puts the text of the clipboard back when it is destroyed. Anything but text is lost.
///
class clipboard_restorer final
{
  // Variables
  const std::optional<std::string> text_;

public: // Structors
  ///
  /// \throws util::exception if the clipboard can not be read, so a test leaves it alone
  ///
  clipboard_restorer();
  ~clipboard_restorer() noexcept;

  clipboard_restorer(const clipboard_restorer&) = delete;
  auto operator=(const clipboard_restorer&) -> clipboard_restorer& = delete;
};

///
///
inline clipboard_restorer::clipboard_restorer()
  : text_{clipboard_text()}
{
  if(!text_)
  {
    throw util::exception{"the clipboard can not be read, it is left alone"};
  }
}

///
///
inline clipboard_restorer::~clipboard_restorer() noexcept
{
  if(text_ && !text_->empty())
  {
    std::ignore = system::clipboard::set_text(*text_);
  }
}

} // namespace bibstd::test_utils
