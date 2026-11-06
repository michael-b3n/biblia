#pragma once

#include <bibstd/bible/reference.hpp>
#include <bibstd/bible/reference_range.hpp>

#include <QString>

#include <optional>

namespace bibstd::bible
{
class versification;
} // namespace bibstd::bible

namespace bibstd::workflow
{
class workflow_scripture;
} // namespace bibstd::workflow

namespace bibqml
{

///
/// Get the name of a book in the default scripture, another of its names or the identifier if it has no short one.
/// Blocks while a script answers.
/// \return book name
///
[[nodiscard]] auto bookName(bibstd::workflow::workflow_scripture& workflowScripture, bibstd::bible::book_id book) -> QString;

///
/// Get the copyright statement of the default scripture. Blocks while a script answers.
/// \return copyright statement, empty if the scripture does not provide one
///
[[nodiscard]] auto scriptureCopyright(bibstd::workflow::workflow_scripture& workflowScripture) -> QString;

///
/// Create a bible reference from QML provided values, validated against the provided versification.
/// \return reference, or std::nullopt if the values do not describe a valid reference
///
[[nodiscard]] auto toReference(const bibstd::bible::versification& versification, const QString& bookId, int chapter, int verse)
  -> std::optional<bibstd::bible::reference>;

///
/// Create a bible reference range from QML provided values, an invalid end falls back to the begin.
/// \return reference range, or std::nullopt if the begin is no valid reference
///
[[nodiscard]] auto toReferenceRange(
  const bibstd::bible::versification& versification,
  const QString& bookId,
  int chapterBegin,
  int verseBegin,
  int chapterEnd,
  int verseEnd
) -> std::optional<bibstd::bible::reference_range>;

} // namespace bibqml
