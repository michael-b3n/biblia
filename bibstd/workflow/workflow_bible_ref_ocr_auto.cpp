#include "bibstd/workflow/workflow_bible_ref_ocr_auto.hpp"
#include "bibstd/framework/setting.hpp"
#include "bibstd/math/rect.hpp"
#include "bibstd/math/value_range.hpp"
#include "bibstd/system/screen.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/format.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/non_owning_ptr.hpp"
#include "bibstd/workflow/workflow_bible_ref_ocr.hpp"
#include "bibstd/workflow/workflow_settings.hpp"

#include <algorithm>
#include <chrono>
#include <expected>
#include <memory>
#include <mutex>
#include <optional>
#include <semaphore>
#include <thread>
#include <tuple>
#include <utility>

namespace bibstd::workflow
{
namespace
{

///
/// Shift a rectangle given in image coordinates onto the screen the image was captured from.
/// \return rectangle in native screen pixels
///
[[nodiscard]] auto to_screen_rect(const util::screen_rect_type& rect, const util::screen_coordinates_type& image_origin)
  -> util::screen_rect_type
{
  return util::screen_rect_type{
    rect.origin() + image_origin, math::size(rect.horizontal_range()), math::size(rect.vertical_range())
  };
}

///
/// Sleep a stop request cuts short, so a run ends without waiting the interval out. Nothing is
/// waited for but that stop, so a semaphore the callback releases needs neither lock nor heap.
///
auto sleep_until_stopped(const std::stop_token& token, const std::chrono::milliseconds duration) -> void
{
  auto wake = std::binary_semaphore{0};
  const auto callback = std::stop_callback{token, [&wake] { wake.release(); }};
  std::ignore = wake.try_acquire_for(duration);
}

} // namespace

///
/// Holds the transition table of the automatic search \see workflow_bible_ref_ocr_auto_sm_builder.
///
class workflow_bible_ref_ocr_auto::machine_holder final
{
  // Typedefs
  using builder_type = detail::workflow_bible_ref_ocr_auto_sm_builder<workflow_bible_ref_ocr_auto>;

  // Variables
  detail::workflow_bible_ref_ocr_auto_sm<workflow_bible_ref_ocr_auto> sm_;

public: // Structors
  explicit machine_holder(workflow_bible_ref_ocr_auto& self)
    : sm_{builder_type::build(self)}
  {
  }

public: // Operators
  ///
  /// Access the state machine. Reached as (*machine_)->, since machine_ is a pointer already and
  /// operator-> stops chaining at the raw one it yields.
  /// \return pointer to the state machine
  ///
  [[nodiscard]] auto operator->() -> util::non_owning_ptr<decltype(sm_)> { return &sm_; }
};

///
///
workflow_bible_ref_ocr_auto_settings::workflow_bible_ref_ocr_auto_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : framework::settings_base{std::move(workflow_settings)}
  , poll_interval{workflow_settings_->create_setting("ocr_auto.poll_interval", std::chrono::milliseconds{250})}
  , dwell_duration{workflow_settings_->create_setting("ocr_auto.dwell_duration", std::chrono::milliseconds{700})}
  , movement_tolerance{workflow_settings_->create_setting("ocr_auto.movement_tolerance", std::int32_t{8}, "px")}
{
}

///
///
workflow_bible_ref_ocr_auto::workflow_bible_ref_ocr_auto(
  std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_bible_ref_ocr> workflow_bible_ref_ocr
)
  : workflow_base{std::move(workflow_settings)}
  , workflow_bible_ref_ocr_{std::move(workflow_bible_ref_ocr)}
  , machine_{std::make_unique<machine_holder>(*this)}
{
}

///
///
workflow_bible_ref_ocr_auto::~workflow_bible_ref_ocr_auto() noexcept = default;

///
///
auto workflow_bible_ref_ocr_auto::start(params params) -> std::expected<void, error_code>
{
  const auto id = params.process_id();
  const auto lock = std::scoped_lock{mtx_};
  if(!(*machine_)->process_event(sm::e_start{id}))
  {
    LOG_WARN("auto reference search start refused: id={}", id);
    return std::unexpected{error_code::invalid_running_state};
  }
  return {};
}

///
///
auto workflow_bible_ref_ocr_auto::stop(params params) -> std::expected<void, error_code>
{
  const auto id = params.process_id();
  const auto lock = std::scoped_lock{mtx_};
  if(!(*machine_)->process_event(sm::e_stop{}))
  {
    LOG_WARN("auto reference search stop refused: id={}", id);
    return std::unexpected{error_code::invalid_running_state};
  }
  return {};
}

///
///
auto workflow_bible_ref_ocr_auto::a_start(const sm::e_start& event, sm::s_running& target) -> void
{
  running_id_ = event.id;
  target.worker = std::jthread{[this, id = event.id](std::stop_token token) { search(std::move(token), id); }};
  LOG_INFO("auto reference search started: id={}", event.id);
}

///
///
auto workflow_bible_ref_ocr_auto::a_stop(sm::s_running& source) -> void
{
  // Move assignment requests the stop and joins, so a search in flight is finished first
  source.worker = std::jthread{};
  LOG_INFO("auto reference search stopped: id={}", running_id_);
}

///
///
auto workflow_bible_ref_ocr_auto::search(const std::stop_token token, const framework::process_id_type id) -> void
{
  auto examined = std::optional<util::screen_coordinates_type>{};
  auto resting_position = std::optional<util::screen_coordinates_type>{};
  auto resting_since = clock_type::time_point{};

  while(!token.stop_requested())
  {
    const auto local = local_settings();
    sleep_until_stopped(token, local.poll_interval);
    if(token.stop_requested())
    {
      break;
    }
    const auto position = system::screen::cursor_position();
    if(!position)
    {
      LOG_WARN("auto reference search failed to identify cursor position");
      continue;
    }

    const auto now = clock_type::now();
    const auto moved = [&](const auto& reference)
    { return util::screen_coordinates_type::distance(reference, *position) > static_cast<double>(local.movement_tolerance); };

    if(!resting_position || moved(*resting_position))
    {
      resting_position = position;
      resting_since = now;
    }
    if(examined && moved(*examined))
    {
      examined.reset();
    }
    if(!examined && now - resting_since >= local.dwell_duration)
    {
      examined = *resting_position;
      examine(token, *position, id);
    }
  }
}

///
///
auto workflow_bible_ref_ocr_auto::examine(
  const std::stop_token& token, const util::screen_coordinates_type position, const framework::process_id_type id
) -> void
{
  try
  {
    const auto detection_id = framework::process_id_type{};
    notify(
      &signals_type::detecting,
      detection_started_type{.process_id = id, .detection_id = detection_id, .cursor_position = position}
    );

    const auto window = system::screen::window_at(position);
    if(!window)
    {
      LOG_DEBUG("auto reference search found no window: position={}", position);
      return;
    }
    auto image = util::pixel_plane_type{};
    if(!system::screen::capture(*window, image))
    {
      LOG_WARN("auto reference search failed to capture screen: position={}", position);
      return;
    }

    auto result = workflow_bible_ref_ocr_->find({
      {.image = image, .position = position - window->origin()}
    });
    if(!result || result->reference_ranges.empty())
    {
      return;
    }
    if(token.stop_requested())
    {
      // The run this result belongs to was stopped while the search was in flight
      LOG_DEBUG("auto reference search discarded result: id={}", id);
      return;
    }

    auto bounding_box = std::optional<util::screen_rect_type>{};
    if(result->reference_bounding_box)
    {
      bounding_box = to_screen_rect(*result->reference_bounding_box, window->origin());
    }
    LOG_INFO(
      "auto reference search detected references: id={}, references=[{}]",
      id,
      util::format::join(result->reference_ranges, ", ")
    );
    notify(
      &signals_type::detected,
      detection_result_type{
        .process_id = id,
        .detection_id = detection_id,
        .reference_ranges = std::move(result->reference_ranges),
        .reference_bounding_box = bounding_box
      }
    );
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
  }
}

///
///
auto workflow_bible_ref_ocr_auto::local_settings() const -> settings_t
{
  return settings_t{
    .poll_interval = std::max(settings().poll_interval->value(), min_poll_interval),
    .dwell_duration = std::max(settings().dwell_duration->value(), min_dwell_duration),
    .movement_tolerance = std::max(settings().movement_tolerance->value(), min_movement_tolerance)
  };
}

} // namespace bibstd::workflow
