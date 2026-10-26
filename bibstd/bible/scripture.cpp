#include "bibstd/bible/scripture.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/ranges.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <set>
#include <vector>

namespace bibstd::bible
{
namespace
{

///
/// \return the default versification whose chapters most often end
/// with the same verse as those of \p passages, the first one on a tie
///
[[nodiscard]] auto closest_default(const scripture::passage_map_type& passages) -> const versification&
{
  const auto same_chapter = [](const reference& a, const reference& b)
  { return a.book() == b.book() && a.chapter() == b.chapter(); };
  const auto chapter_ends = passages | std::views::keys | std::views::chunk_by(same_chapter) |
                            std::views::transform([](const auto& chapter) { return std::ranges::max(chapter); }) |
                            std::ranges::to<std::vector>();
  const auto matches = [&chapter_ends](const versification& v)
  {
    return std::ranges::count_if(
      chapter_ends, [&v](const reference& end) { return v.verse_count(end.book(), end.chapter()) == end.verse().value; }
    );
  };
  return *std::ranges::max_element(versifications_default, {}, matches);
}

///
/// \return the references of \p passages and, for each book they lack, the last verse of each of
/// its chapters in the closest default versification. A warning names the books the scripture \p
/// name lacks. The last verse of a chapter is enough, a versification takes the verses up to it.
///
[[nodiscard]] auto references_of(const std::string& name, const scripture::passage_map_type& passages) -> std::vector<reference>
{
  auto references = passages | std::views::keys | std::ranges::to<std::vector>();
  const auto books = references | std::views::transform(&reference::book) | std::ranges::to<std::set>();
  const auto lacking = util::enum_values<book_id>() |
                       std::views::filter([&books](const book_id book) { return !books.contains(book); }) |
                       std::ranges::to<std::vector>();
  if(lacking.empty())
  {
    return references;
  }
  const auto& closest = closest_default(passages);
  LOG_WARN(
    "scripture lacks books, they take a default versification: name=\"{}\", versification=\"{}\", books={}",
    name,
    closest.name(),
    lacking | std::views::transform([](const book_id book) { return util::enum_name(book); })
  );
  const auto chapter_ends = [&closest](const book_id book)
  {
    const auto chapter_end = [&closest, book](const std::uint32_t chapter_value)
    {
      const auto chapter = reference::chapter_type{chapter_value};
      return reference::create_unguarded(book, chapter, reference::verse_type{closest.verse_count(book, chapter)});
    };
    return util::ranges::index_view_to(closest.chapter_count(book)) |
           std::views::transform([](const auto i) { return i + 1; }) | std::views::transform(chapter_end);
  };
  references.append_range(lacking | std::views::transform(chapter_ends) | std::views::join);
  return references;
}

} // namespace

///
///
scripture::scripture(scripture_info info, book_name_map_type book_name_data, passage_map_type passage_data)
  : info_{std::move(info)}
  , book_name_data_{std::move(book_name_data)}
  , passage_data_{std::move(passage_data)}
  , versification_{[&]
                   {
                     const auto v = bible::versification{info_.name, references_of(info_.name, passage_data_)};
                     const auto it = std::ranges::find(versifications_default, v);
                     return it != std::ranges::cend(versifications_default) ? *it : v;
                   }()}
{
}

///
///
auto scripture::information() const -> const scripture_info&
{
  return info_;
}

///
///
auto scripture::book_information(const book_id book) const -> std::optional<book_name>
{
  const auto it = book_name_data_.find(book);
  return it != std::cend(book_name_data_) ? std::make_optional(it->second) : std::nullopt;
}

///
///
auto scripture::passage(const reference& ref) const -> std::optional<bible::passage>
{
  const auto it = passage_data_.find(ref);
  if(it != std::cend(passage_data_))
  {
    return it->second;
  }
  LOG_DEBUG("passage not found: {}", ref);
  return std::nullopt;
}

///
///
auto scripture::versification() const -> const bible::versification&
{
  return versification_;
}

} // namespace bibstd::bible
