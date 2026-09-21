#pragma once

#include "bibqml/util/ScreenConversion.hpp"

#include <bibstd/bible/reference_range.hpp>
#include <bibstd/framework/process_params.hpp>
#include <bibstd/framework/setting_type_erased.hpp>
#include <bibstd/signal/synchronized_executor.hpp>
#include <bibstd/util/non_owning_ptr.hpp>
#include <bibstd/util/screen_types.hpp>
#include <bibstd/workflow/workflow_hotkey.hpp>

#include <QObject>
#include <QPoint>
#include <QRect>
#include <qtmetamacros.h>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <optional>
#include <string>

namespace bibstd::workflow
{
// Forward declarations
class workflow_bible_ref_ocr;
class workflow_bible_ref_ocr_auto;
class workflow_settings;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// QML bridge for the two reference searches. The manual one is the search the user asks for and
/// is run by this bridge itself, the automatic one is only steered from here. Both report their
/// references through the same signals: a reference the automatic search detected is the same thing
/// as one the user asked for, only the question differs, so the QML layer connects to one place.
///
class BridgeBibleRefOcr final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool manualSearchRunning MEMBER manualSearchRunning_ NOTIFY manualSearchRunningChanged)
  Q_PROPERTY(bool autoSearchRunning MEMBER autoSearchRunning_ NOTIFY autoSearchRunningChanged)
  Q_PROPERTY(QPoint cursorPosition MEMBER cursorPosition_ NOTIFY cursorPositionChanged)

  // Typedefs
  using ClickActionSettingType = bibstd::util::non_owning_ptr<bibstd::framework::setting_type_erased<std::string>>;
  using AutoSearchSettingType = bibstd::util::non_owning_ptr<bibstd::framework::setting_type_erased<bool>>;

  // Variables
  const std::shared_ptr<bibstd::workflow::workflow_bible_ref_ocr> workflowBibleRefOcr_;
  const std::shared_ptr<bibstd::workflow::workflow_bible_ref_ocr_auto> workflowBibleRefOcrAuto_;
  const bibstd::workflow::workflow_hotkey::shared_sig_type manualSearchSig_;
  const ClickActionSettingType clickActionSetting_;
  const AutoSearchSettingType autoSearchSetting_;

  std::optional<bibstd::framework::process_id_type> manualSearchProcessId_;
  std::optional<CursorPositionPair> manualSearchCursor_;
  std::optional<bibstd::framework::process_id_type> autoSearchDetectionId_;
  std::optional<CursorPositionPair> autoSearchCursor_;
  bool manualSearchRunning_{false};
  bool autoSearchRunning_{false};
  QPoint cursorPosition_{0, 0};
  bibstd::signal::synchronized_executor executor_;
  // Carries the auto search requests alone, on a strand of its own: stopping the search joins its
  // run, which must neither happen on the thread of the QML layer nor delay what the other one does
  bibstd::signal::synchronized_executor autoSearchExecutor_;

public: // Typedefs
  ///
  /// Actions that can be executed for a bible reference the user clicks on.
  /// Which action a click executes is up to the user, the value names are the
  /// values the corresponding setting is stored with.
  /// \note The value names must start with an upper case letter. QML does not expose
  /// enum values that start with a lower case letter, they evaluate to undefined there.
  ///
  enum ClickAction
  {
    LookupBrowser,
    None
  };
  Q_ENUM(ClickAction)

public: // Structors
  explicit BridgeBibleRefOcr(
    std::shared_ptr<bibstd::workflow::workflow_bible_ref_ocr> workflowBibleRefOcr,
    std::shared_ptr<bibstd::workflow::workflow_bible_ref_ocr_auto> workflowBibleRefOcrAuto,
    const std::shared_ptr<bibstd::workflow::workflow_hotkey>& workflowHotkey,
    const std::shared_ptr<bibstd::workflow::workflow_settings>& workflowSettings,
    bibstd::util::non_owning_ptr<QObject> parent = nullptr
  );

public: // Accessors
  ///
  /// Access the action configured for a click on a found bible reference.
  /// \return configured click action
  ///
  Q_INVOKABLE ClickAction clickAction() const;

public: // Modifiers
  ///
  /// Ask the automatic reference search to start or to stop. This only states the intent and
  /// returns immediately, the search itself reports its state by the autoSearchRunning property.
  ///
  Q_INVOKABLE void setAutoSearch(bool enabled);

signals:
  // clang-format off
  void manualSearchRunningChanged(bool manualSearchRunning);
  void autoSearchRunningChanged(bool autoSearchRunning);
  void cursorPositionChanged(const QPoint& cursorPosition);
  void referenceFound(const QString& bookId, int chapter, int verse);
  void referenceRangeFound(const QString& bookId, int chapterBegin, int verseBegin, int chapterEnd, int verseEnd, const QRect& boundingBox);
  // clang-format on

public: // Modifiers
  ///
  /// Disconnect all signal connections.
  /// This will stop the frontend backend communication.
  ///
  void disconnect();

private: // Implementation
  void runManualSearch();
  void applyAutoSearch();
  void setManualSearch(std::optional<bibstd::framework::process_id_type> processId);
  void notifyManualSearchStarted(bibstd::framework::process_id_type processId);
  void notifyManualSearchFinished(
    bibstd::framework::process_id_type processId,
    std::optional<bibstd::bible::reference_range> referenceRange,
    std::optional<bibstd::util::screen_rect_type> boundingBox
  );
  void notifyAutoSearchDetecting(bibstd::framework::process_id_type detectionId);
  void notifyAutoSearchDetection(
    bibstd::framework::process_id_type detectionId,
    bibstd::bible::reference_range referenceRange,
    std::optional<bibstd::util::screen_rect_type> boundingBox
  );
  void notifyAutoSearchRunning(bool running);
  void emitCursorPosition(const std::optional<CursorPositionPair>& cursor);
  void emitReference(
    const bibstd::bible::reference_range& referenceRange,
    const std::optional<bibstd::util::screen_rect_type>& boundingBox,
    const std::optional<CursorPositionPair>& cursor
  );
};

} // namespace bibqml
