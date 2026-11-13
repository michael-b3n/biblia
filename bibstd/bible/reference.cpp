#include "bibstd/bible/reference.hpp"
#include "bibstd/bible/versification.hpp"

namespace bibstd::bible
{

///
///
auto reference::create(const book_id book, const chapter_type chapter, const verse_type verse, const versification& validator)
  -> std::optional<reference>
{
  const auto ref = create_unguarded(book, chapter, verse);
  if(!validator.contains(ref))
  {
    return std::nullopt;
  }
  return ref;
}

} // namespace bibstd::bible
