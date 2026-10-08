#pragma once

#include <chrono>
#include <functional>
#include <thread>

namespace bibstd::test_utils
{

///
/// Wait until \p condition holds, the workflows update on threads of their own.
/// \return false if it does not within a few seconds
///
[[nodiscard]] inline auto wait_until(const std::function<bool()>& condition) -> bool
{
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
  while(!condition())
  {
    if(std::chrono::steady_clock::now() > deadline)
    {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  return true;
}

} // namespace bibstd::test_utils
