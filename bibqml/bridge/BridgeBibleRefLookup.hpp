#pragma once

#include <bibstd/bible/reference_range.hpp>
#include <bibstd/framework/process_params.hpp>
#include <bibstd/framework/synchronized_executor.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <QString>
#include <qtmetamacros.h>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <vector>

namespace bibstd::workflow
{
// Forward declarations
class workflow_bible_ref_lookup;
class workflow_scripture;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// QML bridge for workflow_bible_ref_lookup.
/// Opens bible references in the browser.
///
class BridgeBibleRefLookup final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool running MEMBER running_ NOTIFY runningChanged)

  // Variables
  const std::shared_ptr<bibstd::workflow::workflow_bible_ref_lookup> workflowBibleRefLookup_;
  const std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture_;
  bibstd::framework::process_id_type processId_;
  bool running_{false};
  bibstd::framework::synchronized_executor executor_;

public: // Structors
  explicit BridgeBibleRefLookup(
    std::shared_ptr<bibstd::workflow::workflow_bible_ref_lookup> workflowBibleRefLookup,
    std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture,
    bibstd::util::non_owning_ptr<QObject> parent = nullptr
  );
  ~BridgeBibleRefLookup() noexcept override;

signals:
  void runningChanged(bool running);

public: // Modifiers
  ///
  /// Open a bible reference range in the browser.
  ///
  Q_INVOKABLE void lookup(const QString& bookId, int chapterBegin, int verseBegin, int chapterEnd, int verseEnd);

  ///
  /// Open a whole chapter in the browser.
  ///
  Q_INVOKABLE void lookupChapter(const QString& bookId, int chapter);

  ///
  /// Disconnect all signal connections.
  /// This will stop the frontend backend communication.
  ///
  void disconnect();

private: // Implementation
  ///
  /// Start the lookup workflow for the provided reference ranges and track the running state.
  ///
  void start(std::vector<bibstd::bible::reference_range>&& ranges);
};

} // namespace bibqml
