#pragma once

#include "arcstd/util/enum.hpp"
#include "bibstd/bible/reference.hpp"

#include <array>
#include <optional>
#include <ranges>

namespace bibstd::bible
{

///
/// Common functionality for versification classes.
///
struct versification_common final
{
  // Typedefs
  using book_start_indices_type = std::array<std::size_t, arcstd::util::enum_count<book_id>()>;

  // Helpers
  ///
  /// Generates book start indices for the given versification data.
  /// \return book start indices
  ///
  [[nodiscard]] static constexpr auto generate_book_start_indices(const auto& data) -> book_start_indices_type;

  ///
  /// Returns the chapter count of the given book for the given versification data.
  /// \return chapter count of the given book for the given versification data,
  /// or std::nullopt if the book is invalid (non existent)
  ///
  [[nodiscard]] constexpr auto chapter_count(const auto& data, const book_start_indices_type& book_start_indices, book_id book)
    -> std::optional<std::uint32_t>;

  ///
  /// Returns the verse count of the given chapter in the given book for the given versification data.
  /// \return verse count of the given chapter in the given book for the given versification data,
  /// or std::nullopt if the book or chapter is invalid (non existent)
  ///
  [[nodiscard]] constexpr auto verse_count(
    const auto& data, const book_start_indices_type& book_start_indices, book_id book, reference::chapter_type chapter
  ) -> std::optional<std::uint32_t>;
};

///
///
constexpr auto versification_common::generate_book_start_indices(const auto& data) -> book_start_indices_type
{
  auto indices = book_start_indices_type{};
  std::ranges::fill(indices, std::numeric_limits<std::size_t>::max());
  std::ranges::for_each(
    data | std::views::enumerate,
    [&](const auto& p)
    {
      const auto& [i, ref] = p;
      const auto book_index = arcstd::util::to_integral(ref.book());
      indices[book_index] = std::min(indices[book_index], static_cast<std::size_t>(i));
    }
  );
  return indices;
}

///
///
constexpr auto versification_common::chapter_count(
  const auto& data, const book_start_indices_type& book_start_indices, const book_id book
) -> std::optional<std::uint32_t>
{
  if(!arcstd::util::valid(book))
  {
    return std::nullopt;
  }
  const auto book_index = arcstd::util::to_integral(book);
  const auto start_index = book_start_indices[book_index];
  if(start_index < data.size())
  {
    const auto end_index =
      arcstd::util::has_next(book) ? book_start_indices[arcstd::util::to_integral(arcstd::util::next(book))] : data.size();
    if(end_index <= data.size())
    {
      assert(end_index > start_index);
      return end_index - start_index;
    }
  }
  return std::nullopt;
}

///
///
///
constexpr auto versification_common::verse_count(
  const auto& data, const book_start_indices_type& book_start_indices, const book_id book, const reference::chapter_type chapter
) -> std::optional<std::uint32_t>
{
  if(!arcstd::util::valid(book) || chapter == reference::chapter_type{0})
  {
    return std::nullopt;
  }
  const auto book_index = arcstd::util::to_integral(book);
  const auto start_index = book_start_indices[book_index];
  if(start_index < data.size())
  {
    const auto chapter_index = (chapter.value - 1) + start_index;
    if(chapter_index < data.size())
    {
      const auto& ref = data[chapter_index];
      if(ref.book() == book) // ref.chapter() == chapter is given by definition
      {
        return ref.verse().value;
      }
    }
  }
  return std::nullopt;
}

} // namespace bibstd::bible
