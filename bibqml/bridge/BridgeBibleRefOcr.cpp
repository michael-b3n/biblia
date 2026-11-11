#include "bibqml/bridge/BridgeBibleRefOcr.hpp"

#include <arcstd/framework/setting.hpp>
#include <arcstd/math/rect.hpp>
#include <arcstd/system/screen.hpp>
#include <arcstd/util/enum.hpp>
#include <arcstd/util/log.hpp>
#include <arcstd/util/numeric_cast.hpp>
#include <arcstd/util/timer.hpp>
#include <arcstd/workflow/workflow_hotkey.hpp>
#include <arcstd/workflow/workflow_settings.hpp>
#include <bibocr/system/capture.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr_auto.hpp>

#include <QCursor>
#include <QMetaObject>

#include <tuple>
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
  arcstd::data::pixel_plane_type image;
  arcstd::data::screen_coordinates_type relativeCursorPosition;
  arcstd::data::screen_coordinates_type origin;
};

///
/// Read the cursor position in native and device independent pixels at once.
/// \note This accesses the cursor of the QML layer, it must be called on its thread.
/// \return cursor position pair, or std::nullopt if the native cursor position is unknown
///
[[nodiscard]] auto readCursorPositionPair() -> std::optional<arcqml::CursorPositionPair>
{
  const auto native = arcstd::system::screen::cursor_position();
  if(!native)
  {
    LOG_WARN("identify cursor position failed: not found");
    return std::nullopt;
  }
  return arcqml::CursorPositionPair{.native = *native, .deviceIndependent = QCursor::pos()};
}

///
/// Capture the screen area of the window at the specified cursor position.
/// \return captured screen area, or std::nullopt if the area could not be captured
///
[[nodiscard]] auto captureScreen(const arcstd::data::screen_coordinates_type& cursorPosition) -> std::optional<CaptureResult>
{
  SCOPED_TIMER_LOG();
  const auto windowRect = arcstd::system::screen::window_at(cursorPosition);
  if(!windowRect)
  {
    return std::nullopt;
  }
  auto image = arcstd::data::pixel_plane_type{};
  if(!bibocr::system::capture::region(*windowRect, image))
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
  const arcstd::data::screen_rect_type& rect, const arcstd::data::screen_coordinates_type& imageOrigin
) -> arcstd::data::screen_rect_type
{
  return arcstd::data::screen_rect_type{
    rect.origin() + imageOrigin, arcstd::math::size(rect.horizontal_range()), arcstd::math::size(rect.vertical_range())
  };
}

///
/// Create the auto search setting, or access the existing one. Declared here, the backend only searches when told.
/// \return non owning pointer to the auto search setting
///
[[nodiscard]] auto createAutoSearchSetting(arcstd::workflow::workflow_settings& workflowSettings)
  -> arcstd::util::non_owning_ptr<arcstd::framework::setting_type_erased<bool>>
{
  const auto setting = workflowSettings.type_erased_setting(std::string{"ocr.auto_search"}, false);
  return std::get<arcstd::util::non_owning_ptr<arcstd::framework::setting_type_erased<bool>>>(setting);
}

} // namespace

// Constants
constexpr auto manualSearchPath = "manual_reference_search";

///
///
BridgeBibleRefOcr::BridgeBibleRefOcr(
  std::shared_ptr<bibstd::workflow::workflow_bible_ref_ocr> workflowBibleRefOcr,
  std::shared_ptr<bibstd::workflow::workflow_bible_ref_ocr_auto> workflowBibleRefOcrAuto,
  const std::shared_ptr<arcstd::workflow::workflow_hotkey>& workflowHotkey,
  const std::shared_ptr<arcstd::workflow::workflow_settings>& workflowSettings,
  const arcstd::util::non_owning_ptr<QObject> parent
)
  : QObject{parent}
  , workflowBibleRefOcr_{std::move(workflowBibleRefOcr)}
  , workflowBibleRefOcrAuto_{std::move(workflowBibleRefOcrAuto)}
  , manualSearchSig_{workflowHotkey->register_callback(
      manualSearchPath, arcstd::system::hotkey_common::key_modifier::alt, arcstd::system::hotkey_common::key::vk_f
    )}
  , autoSearchSetting_{createAutoSearchSetting(*workflowSettings)}
{
  connections_.connect(*manualSearchSig_, [this]() { runManualSearch(); });

  workflowBibleRefOcrAuto_->connect_queued(
    &bibstd::workflow::workflow_bible_ref_ocr_auto_sigs::detecting,
    [this](const auto& started) { notifyAutoSearchDetecting(started.detection_id); },
    connections_
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
    connections_
  );

  autoSearchSetting_->signal_adapter.connect_queued(
    &arcstd::framework::setting_signals::value_changed, [this]() { applyAutoSearch(); }, connections_
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
void BridgeBibleRefOcr::setAutoSearch(const bool enabled)
{
  // The setting is what the search follows, writing it is what starts and stops it
  std::ignore = autoSearchSetting_->value(enabled);
}

///
///
void BridgeBibleRefOcr::disconnect()
{
  connections_.disconnect();
}

///
///
void BridgeBibleRefOcr::runManualSearch()
{
  // Capture the screen directly on call of this function to ensure the cursor position is up-to-date.
  // This is usually called from the main thread and takes only a few milliseconds.
  // This also ensures that no displayed windows are blocking the screen capture.
  const auto cursorPosition = arcstd::system::screen::cursor_position();
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

  const auto processId = arcstd::framework::process_id_type{};
  notifyManualSearchStarted(processId);

  const auto result = workflowBibleRefOcr_->find({
    {.image = capture->image, .position = capture->relativeCursorPosition}
  });
  if(!result.has_value() || result->reference_ranges.empty())
  {
    notifyManualSearchFinished(processId, std::nullopt, std::nullopt);
    return;
  }

  auto boundingBox = std::optional<arcstd::data::screen_rect_type>{};
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
  const auto lock = std::scoped_lock{autoSearchMtx_};
  const auto enabled = autoSearchSetting_->value();
  const auto result = enabled ? workflowBibleRefOcrAuto_->start({}) : workflowBibleRefOcrAuto_->stop({});
  if(result)
  {
    notifyAutoSearchRunning(enabled);
  }
}

///
///
void BridgeBibleRefOcr::setManualSearch(const std::optional<arcstd::framework::process_id_type> processId)
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
void BridgeBibleRefOcr::notifyManualSearchStarted(const arcstd::framework::process_id_type processId)
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
  const arcstd::framework::process_id_type processId,
  const std::optional<bibstd::bible::reference_range> referenceRange,
  const std::optional<arcstd::data::screen_rect_type> boundingBox
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
void BridgeBibleRefOcr::notifyAutoSearchDetecting(const arcstd::framework::process_id_type detectionId)
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
  const arcstd::framework::process_id_type detectionId,
  const bibstd::bible::reference_range referenceRange,
  const std::optional<arcstd::data::screen_rect_type> boundingBox
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
void BridgeBibleRefOcr::emitCursorPosition(const std::optional<arcqml::CursorPositionPair>& cursor)
{
  cursorPosition_ = cursor ? cursor->deviceIndependent : QCursor::pos();
  emit cursorPositionChanged(cursorPosition_);
}

///
///
void BridgeBibleRefOcr::emitReference(
  const bibstd::bible::reference_range& referenceRange,
  const std::optional<arcstd::data::screen_rect_type>& boundingBox,
  const std::optional<arcqml::CursorPositionPair>& cursor
)
{
  decltype(auto) begin = referenceRange.begin();
  decltype(auto) end = referenceRange.end();
  const auto bookId = QString::fromStdString(std::string{arcstd::util::enum_name(begin.book())});
  const auto area = boundingBox && cursor ? arcqml::toDeviceIndependent(*boundingBox, *cursor) : std::nullopt;

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
