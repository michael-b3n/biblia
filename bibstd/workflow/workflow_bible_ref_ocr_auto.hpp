#pragma once

#include "bibstd/bible/reference_range.hpp"
#include "bibstd/framework/process_params.hpp"
#include "bibstd/framework/settings_base.hpp"
#include "bibstd/signal/adapter.hpp"
#include "bibstd/signal/common.hpp"
#include "bibstd/util/screen_types.hpp"
#include "bibstd/workflow/workflow_base.hpp"

#include <sfsm/sfsm.hpp>

#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

// Forward declarations
namespace bibstd::workflow
{
class workflow_bible_ref_ocr;
} // namespace bibstd::workflow
namespace bibstd::workflow
{
namespace detail
{

// Defined below the workflow, whose actions its transitions run
template<typename Owner>
struct workflow_bible_ref_ocr_auto_sm_builder;

} // namespace detail

///
/// Signals for workflow bible reference ocr auto.
///
struct workflow_bible_ref_ocr_auto_sigs final
{
  // Typedefs
  ///
  /// Error codes emitted by auto bible ref ocr signals.
  ///
  enum class error_code
  {
    invalid_running_state,
  };

  ///
  /// Resting spot the automatic search is about to capture. A detection reported for this spot
  /// carries the same detection ID. The cursor position is given in native pixels of the virtual screen.
  ///
  struct detection_started final
  {
    framework::process_id_type process_id;
    framework::process_id_type detection_id;
    util::screen_coordinates_type cursor_position;
  };

  ///
  /// Bible reference the automatic search detected on the screen, its ranges ordered canonically.
  /// The bounding box is given in native pixels of the virtual screen.
  ///
  struct detection_result final
  {
    framework::process_id_type process_id;
    framework::process_id_type detection_id;
    std::vector<bible::reference_range> reference_ranges;
    std::optional<util::screen_rect_type> reference_bounding_box;
  };

  // Variables
  signal::signal_type<void(const detection_started&)> detecting;
  signal::signal_type<void(const std::expected<detection_result, error_code>&)> detected;
};

///
/// Settings corresponding to workflow bible reference ocr auto.
///
struct workflow_bible_ref_ocr_auto_settings final : public framework::settings_base
{
  // Structors
  workflow_bible_ref_ocr_auto_settings(std::shared_ptr<workflow_settings> workflow_settings);

  // Variables
  const setting_type<std::chrono::milliseconds> poll_interval;
  const setting_type<std::chrono::milliseconds> dwell_duration;
  const setting_type<std::int32_t> movement_tolerance;
};

///
/// Bible reference ocr auto: searches bible references automatically. Resting the cursor
/// on an area triggers the capture of the area and the detection \see workflow_bible_ref_ocr.
///
/// Signal IDs to connect to:
/// - detecting: Emitted for every resting spot before it is captured. Slots receive the start \see detection_started_type.
/// - detected: Emitted for every detected bible reference. Slots receive the detection \see detection_result_type.
///
/// \note detecting and detected are emitted from the thread of the run.
///
class workflow_bible_ref_ocr_auto final
  : public workflow_base<workflow_bible_ref_ocr_auto_settings>
  , public signal::adapter<workflow_bible_ref_ocr_auto_sigs>
{
  // Friends
  template<typename Owner>
  friend struct detail::workflow_bible_ref_ocr_auto_sm_builder;

  // Typedefs
  ///
  /// States and events of the state machine of this workflow.
  ///
  struct sm final
  {
    // clang-format off
    // States
    struct s_idle final {};
    struct s_running final { std::jthread worker; };

    // Events
    struct e_start final { framework::process_id_type id; };
    struct e_stop final {};
    // clang-format on
  };

  using clock_type = std::chrono::steady_clock;
  using error_code = signals_type::error_code;
  using detection_started_type = signals_type::detection_started;
  using detection_result_type = signals_type::detection_result;
  class machine_holder;

  struct settings_t final
  {
    std::chrono::milliseconds poll_interval;
    std::chrono::milliseconds dwell_duration;
    std::int32_t movement_tolerance;
  };

  // Constants
  static constexpr auto min_poll_interval = std::chrono::milliseconds{50};
  static constexpr auto min_dwell_duration = std::chrono::milliseconds{100};
  static constexpr std::int32_t min_movement_tolerance = 0;

  // Variables
  const std::shared_ptr<workflow_bible_ref_ocr> workflow_bible_ref_ocr_;
  mutable std::mutex mtx_;
  framework::process_id_type running_id_;
  // Declared last since its running state holds the thread of a run, which is joined
  // by destroying the machine and must be gone before everything that thread uses.
  const std::unique_ptr<machine_holder> machine_;

public: // Typedefs
  using params = framework::process_params<void>;

public: // Structors
  ///
  /// Constructor for workflow_bible_ref_ocr_auto, starting the search right away if it is enabled.
  ///
  workflow_bible_ref_ocr_auto(
    std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_bible_ref_ocr> workflow_bible_ref_ocr
  );

  ///
  /// Destroying the machine destroys its running state, so a
  /// run on the way is joined here and reports nothing.
  ///
  ~workflow_bible_ref_ocr_auto() noexcept override;

public: // Modifiers
  ///
  /// Start searching the screen. A start while a run is on the way is refused.
  /// \return nothing, or the reason the start was refused
  ///
  [[nodiscard]] auto start(params params) -> std::expected<void, error_code>;

  ///
  /// Stop searching the screen. A stop while no run is on the way is refused.
  /// \note Returns once the search in flight finished.
  /// \return nothing, or the reason the stop was refused
  ///
  [[nodiscard]] auto stop(params params) -> std::expected<void, error_code>;

private: // Actions
  auto a_start(const sm::e_start& event, sm::s_running& target) -> void;
  auto a_stop(sm::s_running& source) -> void;

private: // Implementation
  auto search(std::stop_token token, framework::process_id_type id) -> void;
  auto examine(const std::stop_token& token, util::screen_coordinates_type position, framework::process_id_type id) -> void;
  [[nodiscard]] auto local_settings() const -> settings_t;
};

namespace detail
{

///
/// Builds the transition table of \see workflow_bible_ref_ocr_auto, which starts in s_idle.
/// \note The owner is a template parameter so the table can be built for a stub in a test, and so
/// its actions are only looked up where the owner is complete.
/// \tparam Owner type owning the actions of the transitions
///
template<typename Owner>
struct workflow_bible_ref_ocr_auto_sm_builder final
{
  // Typedefs
  using sm = workflow_bible_ref_ocr_auto::sm;

  ///
  /// \return created state machine
  ///
  [[nodiscard]] static auto build(Owner& self)
  {
    // clang-format off
    return sfsm::sfsm{
      sfsm::states<sm::s_idle, sm::s_running>{{}, {}},
      sfsm::make_transition<sm::s_idle, sm::e_start, sm::s_running>(
        sfsm::action([&self](const sm::e_start& event, sm::s_running& target) { self.a_start(event, target); })
      ),
      sfsm::make_transition<sm::s_running, sm::e_stop, sm::s_idle>(
        sfsm::action([&self](sm::s_running& source) { self.a_stop(source); })
      )
    };
    // clang-format on
  }
};

///
/// Type of the `workflow_bible_ref_ocr_auto` state machine. \see workflow_bible_ref_ocr_auto_sm_builder
///
template<typename Owner>
using workflow_bible_ref_ocr_auto_sm = decltype(workflow_bible_ref_ocr_auto_sm_builder<Owner>::build(std::declval<Owner&>()));

} // namespace detail
} // namespace bibstd::workflow
