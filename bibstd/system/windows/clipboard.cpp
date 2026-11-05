#include "bibstd/system/clipboard.hpp"

#include "bibstd/system/windows/win.hpp"
#include "bibstd/util/scope_guard.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <ranges>
#include <string>
#include <thread>

namespace bibstd::system
{
namespace
{

// Constants
constexpr auto open_attempts = 10;
constexpr auto open_retry_pause = std::chrono::milliseconds{20};

///
/// \return \p text as UTF-16 in memory the clipboard can own, nullptr if there is none
///
[[nodiscard]] auto to_clipboard_memory(const std::string& text) -> HGLOBAL
{
  const auto wide_count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  if(wide_count == 0)
  {
    return nullptr;
  }
  auto wide = std::wstring(static_cast<std::size_t>(wide_count), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), wide_count);

  const auto size = static_cast<std::size_t>(wide_count) * sizeof(wchar_t);
  auto* const memory = GlobalAlloc(GMEM_MOVEABLE, size);
  if(memory == nullptr)
  {
    return nullptr;
  }
  auto* const target = GlobalLock(memory);
  if(target == nullptr)
  {
    GlobalFree(memory);
    return nullptr;
  }
  std::memcpy(target, wide.data(), size);
  GlobalUnlock(memory);
  return memory;
}

///
/// Open the clipboard for \p owner, another program may hold it for a moment.
/// \return true if successful, false otherwise
///
[[nodiscard]] auto open_clipboard(const HWND owner) -> bool
{
  const auto opened = [owner](const int attempt)
  {
    if(OpenClipboard(owner) != 0)
    {
      return true;
    }
    if(attempt + 1 < open_attempts)
    {
      std::this_thread::sleep_for(open_retry_pause);
    }
    return false;
  };
  return std::ranges::any_of(std::views::iota(0, open_attempts), opened);
}

} // namespace

///
///
auto clipboard::set_text(const std::string& text) -> bool
{
  // Without an owner EmptyClipboard makes SetClipboardData fail, \see
  // learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setclipboarddata
  auto* const owner = CreateWindowExW(0, L"STATIC", nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
  if(owner == nullptr)
  {
    return false;
  }
  const auto owner_guard = util::scope_guard{[owner]() { DestroyWindow(owner); }};
  if(!open_clipboard(owner))
  {
    return false;
  }
  const auto clipboard_guard = util::scope_guard{[]() { CloseClipboard(); }};

  auto* const memory = to_clipboard_memory(text);
  if(memory == nullptr)
  {
    return false;
  }
  if(EmptyClipboard() == 0 || SetClipboardData(CF_UNICODETEXT, memory) == nullptr)
  {
    GlobalFree(memory);
    return false;
  }
  return true;
}

} // namespace bibstd::system
