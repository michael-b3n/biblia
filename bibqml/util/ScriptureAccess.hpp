#pragma once

#include <bibstd/bible/reference.hpp>

#include <QString>

#include <optional>

// Forward declarations
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
/// Get the name of a book as provided by the default scripture, in the language of that scripture.
/// Falls back to the raw book identifier if the scripture does not provide a name. Blocks while a script answers.
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

} // namespace bibqml
