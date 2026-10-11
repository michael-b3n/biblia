#include <bibstd/execution/launch.hpp>
#include <bibstd/execution/scope.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace bibstd::execution
{

TEST_CASE("launch_runs_work_on_another_thread", "[execution]")
{
  auto thread = std::thread::id{};
  auto ran = std::atomic_bool{false};
  auto work = scope{};
  const auto note_thread = [&]
  {
    thread = std::this_thread::get_id();
    ran = true;
    ran.notify_all();
  };
  launch(note_thread, work);
  ran.wait(false);
  CHECK(thread != std::this_thread::get_id());
}

TEST_CASE("scope_stop_waits_for_running_work", "[execution]")
{
  auto started = std::atomic_bool{false};
  auto finished = std::atomic_bool{false};
  auto work = scope{};
  const auto slow = [&]
  {
    started = true;
    started.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    finished = true;
  };
  launch(slow, work);
  started.wait(false);
  work.stop();
  CHECK(finished);
}

TEST_CASE("scope_drops_work_launched_after_stop", "[execution]")
{
  auto ran = std::atomic_bool{false};
  auto work = scope{};
  work.stop();
  launch([&] { ran = true; }, work);
  std::this_thread::sleep_for(std::chrono::milliseconds{20});
  CHECK(!ran);
}

TEST_CASE("launch_survives_failing_work", "[execution]")
{
  auto ran = std::atomic_bool{false};
  auto work = scope{};
  launch([] { throw std::runtime_error{"expected by the test"}; }, work);
  const auto succeed = [&]
  {
    ran = true;
    ran.notify_all();
  };
  launch(succeed, work);
  ran.wait(false);
  CHECK(ran);
}

} // namespace bibstd::execution
