#include <arca/signal/adapter.hpp>
#include <arca/signal/connection_store.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibstd::signal
{

///
/// Test signals struct for adapter testing.
///
struct test_signals
{
  signal_type<void()> on_notify;
  signal_type<void(int)> on_value_changed;
  signal_type<void(int, float)> on_multi_args;
};

///
/// Concrete adapter for testing.
/// Exposes notify as public for test purposes.
///
class test_adapter : public adapter<test_signals>
{
public:
  auto fire_notify() -> void { notify(&test_signals::on_notify); }
  auto fire_value_changed(int value) -> void { notify(&test_signals::on_value_changed, value); }
  auto fire_multi_args(int i, float f) -> void { notify(&test_signals::on_multi_args, i, f); }
};

TEST_CASE("adapter connect and notify void signal", "[signal]")
{
  auto a = test_adapter{};
  auto called = false;

  const auto con = a.connect(&test_signals::on_notify, [&called]() { called = true; });

  a.fire_notify();
  CHECK(called);
}

TEST_CASE("adapter connect and notify with argument", "[signal]")
{
  auto a = test_adapter{};
  auto received = 0;

  const auto con = a.connect(&test_signals::on_value_changed, [&received](int v) { received = v; });

  a.fire_value_changed(42);
  CHECK(received == 42);

  a.fire_value_changed(100);
  CHECK(received == 100);
}

TEST_CASE("adapter connect and notify with multiple arguments", "[signal]")
{
  auto a = test_adapter{};
  auto received_int = 0;
  auto received_float = 0.0f;

  const auto con = a.connect(
    &test_signals::on_multi_args,
    [&](int i, float f)
    {
      received_int = i;
      received_float = f;
    }
  );

  a.fire_multi_args(7, 3.14f);
  CHECK(received_int == 7);
  CHECK(received_float == 3.14f);
}

TEST_CASE("adapter multiple slots on same signal", "[signal]")
{
  auto a = test_adapter{};
  auto count = 0;

  const auto con1 = a.connect(&test_signals::on_notify, [&count]() { ++count; });
  const auto con2 = a.connect(&test_signals::on_notify, [&count]() { ++count; });

  a.fire_notify();
  CHECK(count == 2);
}

TEST_CASE("adapter scoped_connection disconnects on destruction", "[signal]")
{
  auto a = test_adapter{};
  auto count = 0;

  {
    const auto con = a.connect(&test_signals::on_notify, [&count]() { ++count; });
    a.fire_notify();
    CHECK(count == 1);
  }

  a.fire_notify();
  CHECK(count == 1);
}

TEST_CASE("adapter connect_extended returns connection", "[signal]")
{
  auto a = test_adapter{};
  auto called = false;

  auto con = a.connect_extended(&test_signals::on_notify, [&called](const connection_type&) { called = true; });

  a.fire_notify();
  CHECK(called);

  con.disconnect();
  called = false;
  a.fire_notify();
  CHECK_FALSE(called);
}

TEST_CASE("adapter independent signals do not interfere", "[signal]")
{
  auto a = test_adapter{};
  auto notify_count = 0;
  auto value_received = 0;

  const auto con1 = a.connect(&test_signals::on_notify, [&notify_count]() { ++notify_count; });
  const auto con2 = a.connect(&test_signals::on_value_changed, [&value_received](int v) { value_received = v; });

  a.fire_notify();
  CHECK(notify_count == 1);
  CHECK(value_received == 0);

  a.fire_value_changed(5);
  CHECK(notify_count == 1);
  CHECK(value_received == 5);
}

TEST_CASE("adapter with connection_store", "[signal]")
{
  auto a = test_adapter{};
  auto count = 0;

  auto store = connection_store{};
  store.store(a.connect(&test_signals::on_notify, [&count]() { ++count; }));
  store.store(a.connect(&test_signals::on_notify, [&count]() { ++count; }));

  a.fire_notify();
  CHECK(count == 2);

  store.clear();
  a.fire_notify();
  CHECK(count == 2);
}

} // namespace bibstd::signal
