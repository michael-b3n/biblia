#include "test_utils/clipboard.hpp"

#include <arcstd/system/windows/win.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <thread>

namespace bibstd::test_utils
{
namespace
{

// Constants
constexpr auto open_attempts = 10;
constexpr auto open_retry_pause = std::chrono::milliseconds{20};

} // namespace

///
///
auto clipboard_text() -> std::optional<std::string>
{
  const auto opened = [](const int attempt)
  {
    if(OpenClipboard(nullptr) != 0)
    {
      return true;
    }
    if(attempt + 1 < open_attempts)
    {
      std::this_thread::sleep_for(open_retry_pause);
    }
    return false;
  };
  if(!std::ranges::any_of(std::views::iota(0, open_attempts), opened))
  {
    return std::nullopt;
  }
  auto result = std::string{};
  if(auto* const memory = GetClipboardData(CF_UNICODETEXT); memory != nullptr)
  {
    const auto* const wide = static_cast<const wchar_t*>(GlobalLock(memory));
    const auto size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    result.resize(static_cast<std::size_t>(size));
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), size, nullptr, nullptr);
    result.pop_back();
    GlobalUnlock(memory);
  }
  CloseClipboard();
  return result;
}

} // namespace bibstd::test_utils
