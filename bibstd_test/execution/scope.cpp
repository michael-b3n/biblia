#include <bibstd/execution/execution.hpp>
#include <bibstd/execution/launch.hpp>
#include <bibstd/execution/scope.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace bibstd::execution
{

TEST_CASE("pool_runs_work_on_another_thread", "[execution]")
{
  const auto result =
    ex::sync_wait(ex::schedule(ex::get_parallel_scheduler()) | ex::then([] { return std::this_thread::get_id(); }));
  REQUIRE(result);
  CHECK(std::get<0>(*result) != std::this_thread::get_id());
}

TEST_CASE("scope_stop_waits_for_running_work", "[execution]")
{
  auto started = std::atomic_bool{false};
  auto finished = std::atomic_bool{false};
  auto work = scope{};
  auto slow = [&]
  {
    started = true;
    started.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    finished = true;
  };
  launch(ex::schedule(ex::get_parallel_scheduler()) | ex::then(std::move(slow)), work);
  started.wait(false);
  work.stop();
  CHECK(finished);
}

TEST_CASE("scope_drops_work_launched_after_stop", "[execution]")
{
  auto ran = std::atomic_bool{false};
  auto work = scope{};
  work.stop();
  launch(ex::schedule(ex::get_parallel_scheduler()) | ex::then([&] { ran = true; }), work);
  std::this_thread::sleep_for(std::chrono::milliseconds{20});
  CHECK(!ran);
}

TEST_CASE("scope_passes_stop_request_to_running_work", "[execution]")
{
  auto started = std::atomic_bool{false};
  auto seen = std::atomic_bool{false};
  auto work = scope{};
  auto wait_for_stop = [&](const ex::inplace_stop_token& token)
  {
    started = true;
    started.notify_all();
    while(!token.stop_requested())
    {
      std::this_thread::yield();
    }
    seen = true;
  };
  launch(
    ex::schedule(ex::get_parallel_scheduler()) | ex::let_value([] { return ex::read_env(ex::get_stop_token); }) |
      ex::then(std::move(wait_for_stop)),
    work
  );
  started.wait(false);
  work.stop();
  CHECK(seen);
}

TEST_CASE("launch_survives_failing_work", "[execution]")
{
  auto ran = std::atomic_bool{false};
  auto work = scope{};
  launch(ex::schedule(ex::get_parallel_scheduler()) | ex::then([] { throw std::runtime_error{"expected by the test"}; }), work);
  auto succeed = [&]
  {
    ran = true;
    ran.notify_all();
  };
  launch(ex::schedule(ex::get_parallel_scheduler()) | ex::then(std::move(succeed)), work);
  ran.wait(false);
  CHECK(ran);
}

} // namespace bibstd::execution
