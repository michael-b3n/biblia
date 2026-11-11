#include <bibstd/workflow/workflow_bible_ref_ocr_auto.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace bibstd::workflow
{
namespace
{

struct actions_stub;

// Typedefs
using sm = detail::workflow_bible_ref_ocr_auto_sm_builder<actions_stub>::sm;

///
/// Owner of the actions of the transitions, standing in for the workflow.
/// Its run is a thread that does nothing but wait for its stop, so the table
/// can be checked without a workflow and without everything a workflow needs to search.
///
struct actions_stub final
{
  // Variables
  std::vector<std::string> ran;
  arcstd::framework::process_id_type running_id;
  std::atomic_bool worker_left{false};

  // Actions
  auto a_start(const sm::e_start& event, sm::s_running& target) -> void
  {
    running_id = event.id;
    target.worker = std::jthread{[this](std::stop_token token)
                                 {
                                   while(!token.stop_requested())
                                   {
                                     std::this_thread::sleep_for(std::chrono::milliseconds{1});
                                   }
                                   worker_left = true;
                                 }};
    ran.emplace_back("a_start");
  }

  auto a_stop(sm::s_running& source) -> void
  {
    source.worker = std::jthread{};
    ran.emplace_back("a_stop");
  }
};

} // namespace

TEST_CASE("workflow_bible_ref_ocr_auto_sm run", "[workflow]")
{
  auto stub = actions_stub{};
  auto machine = detail::workflow_bible_ref_ocr_auto_sm_builder<actions_stub>::build(stub);
  const auto start_id = arcstd::framework::process_id_type{};

  REQUIRE(machine.is_state<sm::s_idle>());

  CHECK(machine.process_event(sm::e_start{start_id}));
  REQUIRE(machine.is_state<sm::s_running>());
  CHECK(stub.running_id == start_id);
  CHECK(machine.state<sm::s_running>().worker.joinable());

  // Stopping joins the run, so the thread of the run is gone by the time the event was answered
  CHECK(machine.process_event(sm::e_stop{}));
  CHECK(machine.is_state<sm::s_idle>());
  CHECK(stub.worker_left);
  CHECK(!machine.state<sm::s_running>().worker.joinable());

  CHECK(stub.ran == std::vector<std::string>{"a_start", "a_stop"});
}

TEST_CASE("workflow_bible_ref_ocr_auto_sm refuses an intent at the wrong time", "[workflow]")
{
  auto stub = actions_stub{};
  auto machine = detail::workflow_bible_ref_ocr_auto_sm_builder<actions_stub>::build(stub);

  SECTION("idle")
  {
    CHECK(!machine.process_event(sm::e_stop{}));
    CHECK(machine.is_state<sm::s_idle>());
    // A refused intent is one no transition matches, so it runs no action either
    CHECK(stub.ran.empty());
  }
  SECTION("running")
  {
    REQUIRE(machine.process_event(sm::e_start{arcstd::framework::process_id_type{}}));
    CHECK(!machine.process_event(sm::e_start{arcstd::framework::process_id_type{}}));
    CHECK(machine.is_state<sm::s_running>());
    CHECK(stub.ran == std::vector<std::string>{"a_start"});
  }
}

TEST_CASE("workflow_bible_ref_ocr_auto_sm keeps the runs apart", "[workflow]")
{
  auto stub = actions_stub{};
  auto machine = detail::workflow_bible_ref_ocr_auto_sm_builder<actions_stub>::build(stub);
  const auto first = arcstd::framework::process_id_type{};

  REQUIRE(machine.process_event(sm::e_start{first}));
  REQUIRE(machine.process_event(sm::e_stop{}));

  const auto second = arcstd::framework::process_id_type{};
  REQUIRE(machine.process_event(sm::e_start{second}));
  CHECK(stub.running_id == second);
  CHECK(stub.running_id != first);
}

} // namespace bibstd::workflow
