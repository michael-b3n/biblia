#include <bibstd/lua/state_owner.hpp>

#include <catch2/catch_test_macros.hpp>

#include <thread>
#include <utility>
#include <vector>

namespace bibstd::lua
{

TEST_CASE("lua_state_owner_setup", "[lua]")
{
  const auto owner = state_owner{};
  const auto s = owner.lock();
  CHECK(s->script("return type(root) == 'table'").get<bool>());
  CHECK(s->script("return string.upper('abc')").get<std::string>() == "ABC");
  CHECK(s->script("return io == nil and os == nil and debug == nil and package == nil").get<bool>());
}

TEST_CASE("lua_state_keeps_the_state_alive", "[lua]")
{
  auto s = state_owner{}.lock();
  CHECK(s->script("return type(root) == 'table'").get<bool>());
}

TEST_CASE("lua_state_owner_keeps_the_state_when_moved", "[lua]")
{
  auto owner = state_owner{};
  const auto moved = std::move(owner);
  // NOLINTNEXTLINE(bugprone-use-after-move) a move is a copy
  owner.lock()->set("value", 1);
  CHECK(moved.lock()->get<int>("value") == 1);
}

TEST_CASE("lua_state_owner_logs", "[lua]")
{
  const auto owner = state_owner{};
  auto s = owner.lock();
  // Scripts have no console, their texts go to the log of the app
  CHECK(
    s.run_script("log.lua", "util.log_debug('text', 1, nil, {}) util.log_info('text') util.log_warning() util.log_error(1)")
  );
}

TEST_CASE("lua_state_owner_copies_share_the_state", "[lua]")
{
  const auto owner = state_owner{};
  const auto copy = owner;
  owner.lock()->set("value", 42);
  CHECK(copy.lock()->get<int>("value") == 42);
}

TEST_CASE("lua_state_owner_is_locked_from_several_threads", "[lua]")
{
  static constexpr auto thread_count = 8;
  static constexpr auto increments = 1000;

  const auto owner = state_owner{};
  owner.lock()->set("counter", 0);
  {
    auto threads = std::vector<std::jthread>{};
    for(auto i = 0; i < thread_count; ++i)
    {
      threads.emplace_back(
        [copy = owner]()
        {
          for(auto j = 0; j < increments; ++j)
          {
            copy.lock()->script("counter = counter + 1");
          }
        }
      );
    }
  }
  CHECK(owner.lock()->get<int>("counter") == thread_count * increments);
}

TEST_CASE("lua_state_owner_is_locked_again_from_a_script", "[lua]")
{
  auto owner = state_owner{};
  {
    auto s = owner.lock();
    s->set("value", 7);
    // A copy kept by the state itself would keep it alive forever
    CHECK(s.register_function("read_value", [&owner]() { return owner.lock()->get<int>("value"); }));
  }
  CHECK(owner.lock()->script("return root.interface.read_value() * 2").get<int>() == 14);
}

} // namespace bibstd::lua
