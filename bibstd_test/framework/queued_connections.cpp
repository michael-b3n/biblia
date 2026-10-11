#include <bibstd/framework/queued_connections.hpp>
#include <bibstd/signal/adapter.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace bibstd::framework
{
namespace
{

struct queued_signals final
{
  signal::signal_type<void()> on_notify;
  signal::signal_type<void(std::string)> on_text;
};

///
/// Adapter exposing notify for the tests.
///
class queued_adapter final : public signal::adapter<queued_signals>
{
public: // Modifiers
  auto fire_notify() -> void;
  auto fire_text(const std::string& text) -> void;
};

///
///
auto queued_adapter::fire_notify() -> void
{
  notify(&queued_signals::on_notify);
}

///
///
auto queued_adapter::fire_text(const std::string& text) -> void
{
  notify(&queued_signals::on_text, text);
}

} // namespace

TEST_CASE("queued_connections_run_the_slot_on_another_thread", "[signal]")
{
  auto adapter = queued_adapter{};
  auto slot_thread = std::thread::id{};
  auto text = std::string{};
  auto called = std::atomic_bool{false};
  auto connections = queued_connections{};
  adapter.connect_queued(
    &queued_signals::on_text,
    [&](const std::string& received)
    {
      slot_thread = std::this_thread::get_id();
      text = received;
      called = true;
      called.notify_all();
    },
    connections
  );
  {
    // Gone before the slot runs, the argument was copied
    const auto temporary = std::string{"copied"};
    adapter.fire_text(temporary);
  }
  called.wait(false);
  CHECK(slot_thread != std::this_thread::get_id());
  CHECK(text == "copied");
}

TEST_CASE("queued_connections_disconnect_waits_for_the_running_slot", "[signal]")
{
  auto adapter = queued_adapter{};
  auto started = std::atomic_bool{false};
  auto finished = std::atomic_bool{false};
  auto connections = queued_connections{};
  adapter.connect_queued(
    &queued_signals::on_notify,
    [&]
    {
      started = true;
      started.notify_all();
      std::this_thread::sleep_for(std::chrono::milliseconds{50});
      finished = true;
    },
    connections
  );
  adapter.fire_notify();
  started.wait(false);
  connections.disconnect();
  CHECK(finished);
}

TEST_CASE("queued_connections_drop_emissions_after_disconnect", "[signal]")
{
  auto adapter = queued_adapter{};
  auto calls = std::atomic_int{0};
  auto connections = queued_connections{};
  adapter.connect_queued(&queued_signals::on_notify, [&] { ++calls; }, connections);
  connections.disconnect();
  adapter.fire_notify();
  CHECK(calls == 0);
}

TEST_CASE("queued_connections_survive_an_emission_after_destruction", "[signal]")
{
  auto adapter = queued_adapter{};
  auto calls = std::atomic_int{0};
  {
    auto connections = queued_connections{};
    adapter.connect_queued(&queued_signals::on_notify, [&] { ++calls; }, connections);
  }
  adapter.fire_notify();
  CHECK(calls == 0);
}

} // namespace bibstd::framework
