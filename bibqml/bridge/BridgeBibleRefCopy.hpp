#pragma once

#include <bibstd/execution/scope.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <QString>
#include <qtmetamacros.h>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace bibstd::workflow
{
class workflow_bible_ref_copy;
class workflow_scripture;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// QML bridge for workflow_bible_ref_copy.
/// Copies bible references to the clipboard.
///
class BridgeBibleRefCopy final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool running MEMBER running_ NOTIFY runningChanged FINAL)

  // Variables
  const std::shared_ptr<bibstd::workflow::workflow_bible_ref_copy> workflowBibleRefCopy_;
  const std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture_;
  bool running_{false};
  bibstd::execution::scope scope_;

public: // Structors
  explicit BridgeBibleRefCopy(
    std::shared_ptr<bibstd::workflow::workflow_bible_ref_copy> workflowBibleRefCopy,
    std::shared_ptr<bibstd::workflow::workflow_scripture> workflowScripture,
    bibstd::util::non_owning_ptr<QObject> parent = nullptr
  );
  ~BridgeBibleRefCopy() noexcept override;

public: // Modifiers
  ///
  /// Copy a bible reference range to the clipboard. A call while another copy runs is ignored, \see copied.
  ///
  Q_INVOKABLE void copy(const QString& bookId, int chapterBegin, int verseBegin, int chapterEnd, int verseEnd);

  ///
  /// This will stop the frontend backend communication.
  ///
  void disconnect();

signals:
  void runningChanged(bool running);
  void copied();
};

} // namespace bibqml
