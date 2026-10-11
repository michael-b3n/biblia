#include <bibstd/system/windows/message_loop.hpp>

#include <bibstd/system/windows/win.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <thread>
#include <vector>

namespace bibstd::system
{

TEST_CASE("message_loop_calls_on_its_thread", "[system]")
{
  auto loop = message_loop{};
  auto first = std::thread::id{};
  auto second = std::thread::id{};
  loop.call([&] { first = std::this_thread::get_id(); });
  loop.call([&] { second = std::this_thread::get_id(); });
  CHECK(first == second);
  CHECK(first != std::thread::id{});
  CHECK(first != std::this_thread::get_id());
}

TEST_CASE("message_loop_runs_posted_tasks_in_order", "[system]")
{
  static constexpr auto count = 100;
  auto order = std::vector<int>{};
  {
    auto loop = message_loop{};
    std::ranges::for_each(std::views::iota(0, count), [&](const auto i) { loop.post([&order, i] { order.push_back(i); }); });
    // Destroyed with tasks posted, all of them run first
  }
  REQUIRE(order.size() == static_cast<std::size_t>(count));
  CHECK(std::ranges::is_sorted(order));
}

TEST_CASE("message_loop_posts_move_only_tasks", "[system]")
{
  auto value = 0;
  {
    auto loop = message_loop{};
    loop.post([&value, held = std::make_unique<int>(42)] { value = *held; });
  }
  CHECK(value == 42);
}

TEST_CASE("message_loop_passes_the_exception_of_a_call", "[system]")
{
  auto loop = message_loop{};
  CHECK_THROWS_AS(loop.call([] { throw std::runtime_error{"expected by the test"}; }), std::runtime_error);
  // A posted task has nobody to report to
  loop.post([] { throw std::runtime_error{"expected by the test"}; });
  auto called = false;
  loop.call([&] { called = true; });
  CHECK(called);
}

TEST_CASE("message_loop_passes_window_messages_to_the_handler", "[system]")
{
  static constexpr auto test_message = UINT{WM_APP + 10};
  auto received = std::atomic<std::uintptr_t>{0};
  auto loop = message_loop{[&](const unsigned int message, const std::uintptr_t wparam)
                           {
                             if(message == test_message)
                             {
                               received = wparam;
                               received.notify_all();
                             }
                           }};
  REQUIRE(PostMessageW(static_cast<HWND>(loop.window()), test_message, 42, 0) != 0);
  received.wait(0);
  CHECK(received == 42);
}

TEST_CASE("message_loop_keeps_running_after_a_quit_of_its_thread", "[system]")
{
  auto loop = message_loop{};
  auto calls = 0;
  // What a tray does when it is closed
  loop.call([] { PostQuitMessage(0); });
  loop.post([&] { ++calls; });
  loop.call([&] { ++calls; });
  loop.call([&] { ++calls; });
  CHECK(calls == 3);
}

} // namespace bibstd::system
