#include <bibstd/execution/execution.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <ranges>
#include <thread>

namespace bibstd::execution
{

// Guards a fix in the vendored library: a callback registered while the stop is requested was lost,
// which leaves whatever waits for that callback waiting forever.
TEST_CASE("stop_callback_registered_while_the_stop_is_requested_is_called", "[execution]")
{
  const auto loses_callback = [](const auto)
  {
    auto source = ex::inplace_stop_source{};
    auto called = std::atomic_bool{false};
    auto go = std::atomic_bool{false};
    auto requesting = std::thread{[&]
                                  {
                                    go.wait(false);
                                    source.request_stop();
                                  }};
    go = true;
    go.notify_all();
    {
      const auto callback = ex::inplace_stop_callback{source.get_token(), [&] { called = true; }};
      requesting.join();
    }
    return !called;
  };
  CHECK(std::ranges::none_of(std::views::iota(0, 20000), loses_callback));
}

} // namespace bibstd::execution
