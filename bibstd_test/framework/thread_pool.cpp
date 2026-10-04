#include <bibstd/framework/thread_pool.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <thread>

namespace bibstd::framework
{

TEST_CASE("thread_pool_shuts_down_while_a_task_runs", "[framework]")
{
  auto finished = std::atomic_bool{false};
  {
    const auto guard = thread_pool::init();
    thread_pool::queue_task(
      [&]
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        finished = true;
      }
    );
    // Running when the guard releases the pool
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
  }
  CHECK(finished);
}

} // namespace bibstd::framework
