#include "bibqml/bridge/BridgeBibleRefOcr.hpp"

#include <bibstd/framework/setting.hpp>
#include <bibstd/framework/thread_pool.hpp>
#include <bibstd/math/rect.hpp>
#include <bibstd/system/screen.hpp>
#include <bibstd/util/enum.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/util/numeric_cast.hpp>
#include <bibstd/util/timer.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr_auto.hpp>
#include <bibstd/workflow/workflow_hotkey.hpp>
#include <bibstd/workflow/workflow_settings.hpp>

#include <QCursor>
#include <QMetaObject>

#include <ranges>
#include <utility>

namespace bibqml
{
namespace
{

///
/// Screen area captured for a reference search. Besides the captured image this contains the
/// cursor position relative to the image origin and the origin of the image on the screen.
///
struct CaptureResult final
{
  bibstd::util::pixel_plane_type image;
  bibstd::util::screen_coordinates_type relativeCursorPosition;
  bibstd::util::screen_coordinates_type origin;
};

///
/// Read the cursor position in native and device independent pixels at once.
/// \note This accesses the cursor of the QML layer, it must be called on its thread.
/// \return cursor position pair, or std::nullopt if the native cursor position is unknown
///
[[nodiscard]] auto readCursorPositionPair() -> std::optional<CursorPositionPair>
{
  const auto native = bibstd::system::screen::cursor_position();
  if(!native)
  {
    LOG_WARN("identify cursor position failed: not found");
    return std::nullopt;
  }
  return CursorPositionPair{.native = *native, .deviceIndependent = QCursor::pos()};
}

///
/// Capture the screen area of the window at the specified cursor position.
/// \return captured screen area, or std::nullopt if the area could not be captured
///
[[nodiscard]] auto captureScreen(const bibstd::util::screen_coordinates_type& cursorPosition) -> std::optional<CaptureResult>
{
  SCOPED_TIMER_LOG();
  const auto windowRect = bibstd::system::screen::window_at(cursorPosition);
  if(!windowRect)
  {
    return std::nullopt;
  }
  auto image = bibstd::util::pixel_plane_type{};
  if(!bibstd::system::screen::capture(*windowRect, image))
  {
    return std::nullopt;
  }
  return CaptureResult{
    .image = std::move(image), .relativeCursorPosition = cursorPosition - windowRect->origin(), .origin = windowRect->origin()
  };
}

///
/// Shift a rectangle given in image coordinates onto the screen the image was captured from.
/// \return rectangle in native screen pixels
///
[[nodiscard]] auto toScreenRect(
  const bibstd::util::screen_rect_type& rect, const bibstd::util::screen_coordinates_type& imageOrigin
) -> bibstd::util::screen_rect_type
{
  return bibstd::util::screen_rect_type{
    rect.origin() + imageOrigin, bibstd::math::size(rect.horizontal_range()), bibstd::math::size(rect.vertical_range())
  };
}

///
/// Create the click action setting, or access it if it exists already. The setting is declared
/// by the frontend since the available actions are a frontend concept: the backend neither
/// knows nor cares which of them is bound to a click.
/// \return non owning pointer to the click action setting
///
[[nodiscard]] auto createClickActionSetting(bibstd::workflow::workflow_settings& workflowSettings)
  -> bibstd::util::non_owning_ptr<bibstd::framework::setting_type_erased<std::string>>
{
  using ClickAction = BridgeBibleRefOcr::ClickAction;
  const auto available = bibstd::util::enum_names<ClickAction>() |
                         std::views::transform([](const auto name) { return std::string{name}; }) |
                         std::ranges::to<std::vector>();
  const auto setting = workflowSettings.type_erased_setting(
    std::string{"ocr.click_action"},
    std::string{bibstd::util::enum_name(ClickAction::LookupBrowser)},
    std::make_shared<bibstd::framework::setting_validator_list<std::string>>(available)
  );
  return std::get<bibstd::util::non_owning_ptr<bibstd::framework::setting_type_erased<std::string>>>(setting);
}

///
/// Create the auto search setting, or access it if it exists already. The setting is declared by
/// the frontend because it is the frontend that states the intent: the backend searches when it is
/// told to and knows nothing about what the user asked for last time.
/// \return non owning pointer to the auto search setting
///
[[nodiscard]] auto createAutoSearchSetting(bibstd::workflow::workflow_settings& workflowSettings)
  -> bibstd::util::non_owning_ptr<bibstd::framework::setting_type_erased<bool>>
{
  const auto setting = workflowSettings.type_erased_setting(std::string{"ocr.auto_search"}, false);
  return std::get<bibstd::util::non_owning_ptr<bibstd::framework::setting_type_erased<bool>>>(setting);
}

} // namespace

// Constants
constexpr auto ocrFindPath = "ocr";

///
///
BridgeBibleRefOcr::BridgeBibleRefOcr(
  std::shared_ptr<bibstd::workflow::workflow_bible_ref_ocr> workflowBibleRefOcr,
  std::shared_ptr<bibstd::workflow::workflow_bible_ref_ocr_auto> workflowBibleRefOcrAuto,
  const std::shared_ptr<bibstd::workflow::workflow_hotkey>& workflowHotkey,
  const std::shared_ptr<bibstd::workflow::workflow_settings>& workflowSettings,
  const bibstd::util::non_owning_ptr<QObject> parent
)
  : QObject{parent}
  , workflowBibleRefOcr_{std::move(workflowBibleRefOcr)}
  , workflowBibleRefOcrAuto_{std::move(workflowBibleRefOcrAuto)}
  , manualSearchSig_{workflowHotkey->register_callback(
      ocrFindPath, bibstd::system::hotkey_common::key_modifier::alt, bibstd::system::hotkey_common::key::vk_f
    )}
  , clickActionSetting_{createClickActionSetting(*workflowSettings)}
  , autoSearchSetting_{createAutoSearchSetting(*workflowSettings)}
  , autoSearchExecutor_{bibstd::framework::thread_pool::strand_id()}
{
  executor_.connect(*manualSearchSig_, [this]() { runManualSearch(); });

  workflowBibleRefOcrAuto_->connect_queued(
    &bibstd::workflow::workflow_bible_ref_ocr_auto_sigs::detecting,
    [this](const auto& started) { notifyAutoSearchDetecting(started.detection_id); },
    executor_
  );
  workflowBibleRefOcrAuto_->connect_queued(
    &bibstd::workflow::workflow_bible_ref_ocr_auto_sigs::detected,
    [this](const auto& detected)
    {
      // The ranges are ordered canonically, the first one is the reference that was detected.
      if(detected && !detected->reference_ranges.empty())
      {
        notifyAutoSearchDetection(detected->detection_id, detected->reference_ranges.front(), detected->reference_bounding_box);
      }
    },
    executor_
  );

  autoSearchSetting_->signal_adapter.connect_queued(
    &bibstd::framework::setting_signals::value_changed, [this]() { applyAutoSearch(); }, autoSearchExecutor_
  );
  // An unchanged setting emits nothing, so a search left on is started here. Starting is the one
  // half that does not wait for a run, so it needs no thread of its own
  if(autoSearchSetting_->value())
  {
    applyAutoSearch();
  }
}

///
///
BridgeBibleRefOcr::ClickAction BridgeBibleRefOcr::clickAction() const
{
  const auto action = bibstd::util::to_enum<ClickAction>(clickActionSetting_->value());
  if(!action)
  {
    // A value that cannot be read, e.g. one persisted by an older version, falls back to the
    // default action. Reporting no action instead would leave a click without any effect.
    LOG_WARN("unknown click action configured: value=\"{}\"", clickActionSetting_->value());
    return ClickAction::LookupBrowser;
  }
  return *action;
}

///
///
void BridgeBibleRefOcr::setAutoSearch(const bool enabled)
{
  // The setting is what the search follows, writing it is what starts and stops it
  autoSearchSetting_->value(enabled);
}

///
///
void BridgeBibleRefOcr::disconnect()
{
  executor_.disconnect();
  autoSearchExecutor_.disconnect();
}

///
///
void BridgeBibleRefOcr::runManualSearch()
{
  // Capture the screen directly on call of this function to ensure the cursor position is up-to-date.
  // This is usually called from the main thread and takes only a few milliseconds.
  // This also ensures that no displayed windows are blocking the screen capture.
  const auto cursorPosition = bibstd::system::screen::cursor_position();
  if(!cursorPosition)
  {
    LOG_WARN("identify cursor position failed: not found");
    return;
  }
  const auto capture = captureScreen(*cursorPosition);
  if(!capture)
  {
    LOG_WARN("capture screen failed: cursor_position={}", *cursorPosition);
    return;
  }

  const auto processId = bibstd::framework::process_id_type{};
  notifyManualSearchStarted(processId);

  const auto result = workflowBibleRefOcr_->find({
    {.image = capture->image, .position = capture->relativeCursorPosition}
  });
  if(!result.has_value() || result->reference_ranges.empty())
  {
    notifyManualSearchFinished(processId, std::nullopt, std::nullopt);
    return;
  }

  auto boundingBox = std::optional<bibstd::util::screen_rect_type>{};
  if(result->reference_bounding_box)
  {
    boundingBox = toScreenRect(*result->reference_bounding_box, capture->origin);
  }
  // The ranges are ordered canonically, the first one is the reference the passage belongs to.
  notifyManualSearchFinished(processId, result->reference_ranges.front(), boundingBox);
}

///
///
void BridgeBibleRefOcr::applyAutoSearch()
{
  const auto enabled = autoSearchSetting_->value();
  const auto result = enabled ? workflowBibleRefOcrAuto_->start({}) : workflowBibleRefOcrAuto_->stop({});
  if(result)
  {
    notifyAutoSearchRunning(enabled);
  }
}

///
///
void BridgeBibleRefOcr::setManualSearch(const std::optional<bibstd::framework::process_id_type> processId)
{
  manualSearchProcessId_ = processId;
  const auto running = manualSearchProcessId_.has_value();
  if(manualSearchRunning_ != running)
  {
    manualSearchRunning_ = running;
    emit manualSearchRunningChanged(manualSearchRunning_);
  }
}

///
///
void BridgeBibleRefOcr::notifyManualSearchStarted(const bibstd::framework::process_id_type processId)
{
  QMetaObject::invokeMethod(
    this,
    [this, processId]()
    {
      setManualSearch(processId);
      // Read right after the capture, the cursor is still where the search runs
      manualSearchCursor_ = readCursorPositionPair();
      emitCursorPosition(manualSearchCursor_);
    },
    Qt::QueuedConnection
  );
}

///
///
void BridgeBibleRefOcr::notifyManualSearchFinished(
  const bibstd::framework::process_id_type processId,
  const std::optional<bibstd::bible::reference_range> referenceRange,
  const std::optional<bibstd::util::screen_rect_type> boundingBox
)
{
  QMetaObject::invokeMethod(
    this,
    [this, processId, referenceRange, boundingBox]()
    {
      // A newer search took over in the meantime, only its result is of interest.
      if(manualSearchProcessId_ != processId)
      {
        return;
      }
      setManualSearch(std::nullopt);

      if(referenceRange)
      {
        emitReference(*referenceRange, boundingBox, manualSearchCursor_);
      }
    },
    Qt::QueuedConnection
  );
}

///
///
void BridgeBibleRefOcr::notifyAutoSearchDetecting(const bibstd::framework::process_id_type detectionId)
{
  QMetaObject::invokeMethod(
    this,
    [this, detectionId]()
    {
      // Read before the search runs, the cursor is still on the monitor being examined
      autoSearchDetectionId_ = detectionId;
      autoSearchCursor_ = readCursorPositionPair();
    },
    Qt::QueuedConnection
  );
}

///
///
void BridgeBibleRefOcr::notifyAutoSearchDetection(
  const bibstd::framework::process_id_type detectionId,
  const bibstd::bible::reference_range referenceRange,
  const std::optional<bibstd::util::screen_rect_type> boundingBox
)
{
  QMetaObject::invokeMethod(
    this,
    [this, detectionId, referenceRange, boundingBox]()
    {
      // A manual search that is still in flight is left behind by this detection, so it is no
      // longer the current one and its result is dropped when it arrives
      setManualSearch(std::nullopt);
      // The signals are not ordered, the current cursor stands in if the detection is not known yet
      const auto cursor = autoSearchDetectionId_ == detectionId ? autoSearchCursor_ : readCursorPositionPair();
      emitCursorPosition(cursor);
      emitReference(referenceRange, boundingBox, cursor);
    },
    Qt::QueuedConnection
  );
}

///
///
void BridgeBibleRefOcr::notifyAutoSearchRunning(const bool running)
{
  QMetaObject::invokeMethod(
    this,
    [this, running]()
    {
      if(autoSearchRunning_ != running)
      {
        autoSearchRunning_ = running;
        emit autoSearchRunningChanged(autoSearchRunning_);
      }
    },
    Qt::QueuedConnection
  );
}

///
///
void BridgeBibleRefOcr::emitCursorPosition(const std::optional<CursorPositionPair>& cursor)
{
  cursorPosition_ = cursor ? cursor->deviceIndependent : QCursor::pos();
  emit cursorPositionChanged(cursorPosition_);
}

///
///
void BridgeBibleRefOcr::emitReference(
  const bibstd::bible::reference_range& referenceRange,
  const std::optional<bibstd::util::screen_rect_type>& boundingBox,
  const std::optional<CursorPositionPair>& cursor
)
{
  decltype(auto) begin = referenceRange.begin();
  decltype(auto) end = referenceRange.end();
  const auto bookId = QString::fromStdString(std::string{bibstd::util::enum_name(begin.book())});
  const auto area = boundingBox && cursor ? toDeviceIndependent(*boundingBox, *cursor) : std::nullopt;

  emit referenceFound(bookId, numeric_cast<int>(begin.chapter().value), numeric_cast<int>(begin.verse().value));
  emit referenceRangeFound(
    bookId,
    numeric_cast<int>(begin.chapter().value),
    numeric_cast<int>(begin.verse().value),
    numeric_cast<int>(end.chapter().value),
    numeric_cast<int>(end.verse().value),
    area.value_or(QRect{})
  );
}

} // namespace bibqml
