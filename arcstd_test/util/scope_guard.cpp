#include <arcstd/util/scope_guard.hpp>

#include <catch2/catch_test_macros.hpp>

#include <utility>
#include <vector>

namespace bibstd::util
{

TEST_CASE("scope_guard calls the callable on destruction", "[util]")
{
  auto called = 0;
  {
    const auto guard = scope_guard{[&]() { ++called; }};
    CHECK(called == 0);
  }
  CHECK(called == 1);
}

TEST_CASE("scope_guard default constructed does nothing", "[util]")
{
  auto guard = scope_guard{};
  CHECK_NOTHROW(guard.reset());
}

TEST_CASE("scope_guard reset calls the callable once", "[util]")
{
  auto called = 0;
  auto guard = scope_guard{[&]() { ++called; }};
  guard.reset();
  CHECK(called == 1);
  // A second reset must not call the callable again.
  guard.reset();
  CHECK(called == 1);
}

TEST_CASE("scope_guard reset prevents the call on destruction", "[util]")
{
  auto called = 0;
  {
    auto guard = scope_guard{[&]() { ++called; }};
    guard.reset();
    CHECK(called == 1);
  }
  CHECK(called == 1);
}

TEST_CASE("scope_guard move transfers the callable", "[util]")
{
  auto called = 0;
  {
    auto moved_to = scope_guard{};
    {
      auto guard = scope_guard{[&]() { ++called; }};
      moved_to = std::move(guard);
      CHECK(called == 0);
    }
    // The moved from guard must not call the callable on its destruction.
    CHECK(called == 0);
  }
  CHECK(called == 1);
}

TEST_CASE("scope_guard move construction transfers the callable", "[util]")
{
  auto called = 0;
  {
    auto guard = scope_guard{[&]() { ++called; }};
    const auto moved_to = scope_guard{std::move(guard)};
    CHECK(called == 0);
  }
  CHECK(called == 1);
}

TEST_CASE("scope_guard move assignment resets the previous callable", "[util]")
{
  auto previous_called = 0;
  auto next_called = 0;
  {
    auto guard = scope_guard{[&]() { ++previous_called; }};
    guard = scope_guard{[&]() { ++next_called; }};
    CHECK(previous_called == 1);
    CHECK(next_called == 0);
  }
  CHECK(previous_called == 1);
  CHECK(next_called == 1);
}

TEST_CASE("scope_guard destruction order is reverse", "[util]")
{
  auto order = std::vector<int>{};
  {
    const auto first = scope_guard{[&]() { order.push_back(1); }};
    const auto second = scope_guard{[&]() { order.push_back(2); }};
  }
  CHECK(order == std::vector<int>{2, 1});
}

TEST_CASE("shared_scope_guard calls the callable once all guards are gone", "[util]")
{
  auto called = 0;
  auto creator = shared_scope_guard::creator{};
  {
    const auto first = creator.create([&]() { ++called; });
    {
      const auto second = creator.create([&]() { ++called; });
      CHECK(called == 0);
    }
    // The callable must survive as long as one guard is alive.
    CHECK(called == 0);
  }
  CHECK(called == 1);
}

TEST_CASE("shared_scope_guard reports the initial instance", "[util]")
{
  auto creator = shared_scope_guard::creator{};
  const auto first = creator.create([]() {});
  const auto second = creator.create([]() {});
  CHECK(first.is_initial_instance());
  CHECK(!second.is_initial_instance());
}

TEST_CASE("shared_scope_guard creates a new instance once all guards are gone", "[util]")
{
  auto called = 0;
  auto creator = shared_scope_guard::creator{};
  {
    const auto first = creator.create([&]() { ++called; });
    CHECK(first.is_initial_instance());
  }
  CHECK(called == 1);
  {
    const auto next = creator.create([&]() { ++called; });
    // The previous instance expired, so this guard holds a fresh callable.
    CHECK(next.is_initial_instance());
  }
  CHECK(called == 2);
}

TEST_CASE("shared_scope_guard reset releases the guard", "[util]")
{
  auto called = 0;
  auto creator = shared_scope_guard::creator{};
  auto first = creator.create([&]() { ++called; });
  auto second = creator.create([&]() { ++called; });
  first.reset();
  CHECK(called == 0);
  second.reset();
  CHECK(called == 1);
}

TEST_CASE("shared_scope_guard default constructed does nothing", "[util]")
{
  auto guard = shared_scope_guard{};
  CHECK(!guard.is_initial_instance());
  CHECK_NOTHROW(guard.reset());
}

} // namespace bibstd::util
