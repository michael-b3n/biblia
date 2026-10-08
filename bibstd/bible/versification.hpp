#pragma once

#include "bibstd/bible/reference.hpp"
#include "bibstd/bible/reference_range.hpp"
#include "bibstd/bible/versification_common.hpp"
#include "bibstd/bible/versification_default_esv.hpp"
#include "bibstd/bible/versification_default_kjv.hpp"
#include "bibstd/meta/contains.hpp"
#include "bibstd/meta/pack.hpp"
#include "bibstd/util/ranges.hpp"

#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <variant>
#include <vector>

namespace bibstd::bible
{

///
/// Versification class.
/// Once constructed, the data within the versification
/// is immutable, and all operations are thread-safe.
///
class versification final
{
  // Typedefs
  using book_start_indices_type = versification_common::book_start_indices_type;
  struct versification_dynamic final
  {
    std::string name;
    std::vector<reference> data;
    book_start_indices_type book_start_indices;
    std::uint32_t reference_count;
  };
  using data_type = std::variant<versification_dynamic, versification_default_esv, versification_default_kjv>;

  // Variables
  const data_type data_;

public: // Typedefs
  using default_esv = versification_default_esv;
  using default_kjv = versification_default_kjv;

  using all_defaults_variant = std::variant<default_esv, default_kjv>;

public: // Structors
  ///
  /// Creates a reference validator for the default Bible version (ESV).
  ///
  constexpr versification(auto&& versification_default)
    requires(meta::contains_v<all_defaults_variant, std::decay_t<decltype(versification_default)>>);

  ///
  /// Creates a versification for the given Bible version.
  ///
  versification(std::string_view name, const std::vector<reference>& references);

  ///
  /// Destructor.
  ///
  ~versification() noexcept = default;

public: // Operators
  ///
  /// Checks if this versification is equal to another versification.
  /// Names are not compared, only the references are compared.
  /// This is an expensive operation that compares all references of the two versifications.
  /// \return true if the versifications are equal, false otherwise
  ///
  constexpr auto operator==(const versification& other) const -> bool;

public: // Accessors
  ///
  /// Returns the name of the versification.
  /// \return name of the versification
  ///
  constexpr auto name() const -> std::string;

  ///
  /// Get the count of chapters in a book.
  /// \return the chapter count, can be zero on invalid arguments
  ///
  constexpr auto chapter_count(book_id book) const -> std::uint32_t;

  ///
  /// Get the count of verses in a chapter.
  /// \return the verse count, can be zero on invalid arguments
  ///
  constexpr auto verse_count(book_id book, reference::chapter_type chapter) const -> std::uint32_t;

  ///
  /// Returns the number of references according to the validator.
  /// \return number of references
  ///
  constexpr auto count() const -> std::uint32_t;

  ///
  /// Returns the size of the given reference range.
  /// \return size of the reference range if it exists, std::nullopt otherwise
  ///
  constexpr auto size(const reference_range& ref) const -> std::optional<std::uint32_t>;

  ///
  /// Checks if the given reference is valid.
  /// \return true if the reference is valid, false otherwise
  ///
  constexpr auto contains(const reference& ref) const -> bool;

  ///
  /// Validates the given reference by returning the reference
  /// with the chapter and verse numbers clamped to the valid range.
  /// \return validated reference
  ///
  constexpr auto validate(const reference& ref) const -> reference;

public: // Operations
  ///
  /// Returns the next reference after the given reference.
  /// \return next reference if it exists, std::nullopt otherwise
  ///
  constexpr auto next(const reference& ref) const -> std::optional<reference>;

  ///
  /// Returns the previous reference before the given reference.
  /// \return previous reference if it exists, std::nullopt otherwise
  ///
  constexpr auto prev(const reference& ref) const -> std::optional<reference>;

  ///
  /// Splits the given reference range by chapter. A chapter the range runs through
  /// is taken from its first to its last verse.
  /// \return reference range of each chapter, in order
  ///
  constexpr auto split_by_chapter(const reference_range& ref) const -> std::vector<reference_range>;

private: // Helpers
  static constexpr auto visit_size(const data_type& data) -> std::size_t;
  static constexpr auto visit_last_reference(const data_type& data, std::size_t index) -> std::optional<reference>;
  static constexpr auto visit_book_start_indices(const data_type& data) -> const book_start_indices_type&;

private: // Implementation
  constexpr auto containing_index(const reference& ref) const -> std::optional<std::size_t>;
  constexpr auto containing_index(book_id book, reference::chapter_type chapter) const -> std::optional<std::size_t>;
};

///
///
constexpr versification::versification(auto&& versification_default)
  requires(meta::contains_v<all_defaults_variant, std::decay_t<decltype(versification_default)>>)
  : data_{std::forward<decltype(versification_default)>(versification_default)}
{
}

///
///
constexpr auto versification::operator==(const versification& other) const -> bool
{
  if(this == &other)
  {
    return true;
  }
  const auto data_size = visit_size(data_);
  if(data_size != visit_size(other.data_))
  {
    return false;
  }
  return std::ranges::all_of(
    util::ranges::index_view_to(data_size),
    [&](const auto i) { return visit_last_reference(data_, i) == visit_last_reference(other.data_, i); }
  );
}

///
///
constexpr auto versification::name() const -> std::string
{
  return std::visit([](const auto& d) { return std::string{d.name}; }, data_);
}

///
///
constexpr auto versification::chapter_count(const book_id book) const -> std::uint32_t
{
  if(!util::valid(book))
  {
    return 0;
  }
  else if(util::has_next(book))
  {
    const auto next_book_start_index = visit_book_start_indices(data_).at(util::to_integral(util::next(book)));
    const auto ref = visit_last_reference(data_, next_book_start_index - 1);
    return ref ? ref->chapter().value : 0;
  }
  else
  {
    static constexpr auto visit_last_chapter_value = [](const auto& data) -> std::uint32_t
    { return std::visit([](const auto& d) { return d.data.back().chapter().value; }, data); };
    return visit_last_chapter_value(data_);
  }
}

///
///
constexpr auto versification::verse_count(const book_id book, const reference::chapter_type chapter) const -> std::uint32_t
{
  if(const auto index = containing_index(book, chapter))
  {
    if(const auto ref = visit_last_reference(data_, *index))
    {
      return ref->verse().value;
    }
  }
  return 0;
}

///
///
constexpr auto versification::count() const -> std::uint32_t
{
  return std::visit([](const auto& d) { return d.reference_count; }, data_);
}

///
///
constexpr auto versification::size(const reference_range& ref) const -> std::optional<std::uint32_t>
{
  static constexpr auto is_invalid = [](const auto& r)
  { return !util::valid(r.book()) || r.chapter() == decltype(r.chapter()){0} || r.verse() == decltype(r.verse()){0}; };

  if(is_invalid(ref.begin()) || is_invalid(ref.end()))
  {
    return std::nullopt;
  }
  const auto begin_chapter_index = containing_index(ref.begin());
  const auto end_chapter_index = containing_index(ref.end());
  if(!begin_chapter_index || !end_chapter_index)
  {
    return std::nullopt;
  }

  auto result = std::optional<std::uint32_t>{};
  if(const auto begin_chapter_ref = visit_last_reference(data_, *begin_chapter_index))
  {
    if(*begin_chapter_index == *end_chapter_index)
    {
      result = ref.end().verse().value - ref.begin().verse().value + 1;
    }
    else if(const auto end_chapter_ref = visit_last_reference(data_, *end_chapter_index))
    {
      assert(begin_chapter_ref->verse().value >= ref.begin().verse().value);
      result = (begin_chapter_ref->verse().value - ref.begin().verse().value + 1) + (ref.end().verse().value);
      if(*end_chapter_index > *begin_chapter_index)
      {
        result = std::ranges::fold_left(
          util::ranges::index_view_between(*begin_chapter_index + 1, *end_chapter_index),
          *result,
          [&](const auto n, const auto i) { return n + visit_last_reference(data_, i).value().verse().value; }
        );
      }
    }
  }
  return result;
}

///
///
constexpr auto versification::contains(const reference& ref) const -> bool
{
  return containing_index(ref).has_value();
}

///
///
constexpr auto versification::validate(const reference& ref) const -> reference
{
  if(contains(ref))
  {
    return ref;
  }
  const auto book = std::clamp(ref.book(), book_id::genesis, book_id::revelation);
  const auto chapter = std::clamp(ref.chapter(), reference::chapter_type{1}, reference::chapter_type{chapter_count(book)});
  const auto verse = std::clamp(ref.verse(), reference::verse_type{1}, reference::verse_type{verse_count(book, chapter)});
  return reference::create_unguarded(book, chapter, verse);
}

///
///
constexpr auto versification::next(const reference& ref) const -> std::optional<reference>
{
  if(!containing_index(ref).has_value())
  {
    return std::nullopt;
  }
  // At this point we know the reference is valid.
  auto book = ref.book();
  auto chapter = ref.chapter();
  auto verse = ref.verse();
  if(verse < decltype(verse){verse_count(book, chapter)})
  {
    verse = decltype(verse){verse.value + 1};
  }
  else if(chapter < decltype(chapter){chapter_count(book)})
  {
    chapter = decltype(chapter){chapter.value + 1};
    verse = decltype(verse){1};
  }
  else if(util::has_next(book))
  {
    book = util::next(book);
    chapter = decltype(chapter){1};
    verse = decltype(verse){1};
  }
  else
  {
    return std::nullopt;
  }
  return reference::create_unguarded(book, chapter, verse);
}

///
///
constexpr auto versification::prev(const reference& ref) const -> std::optional<reference>
{
  if(!containing_index(ref).has_value())
  {
    return std::nullopt;
  }
  // At this point we know the reference is valid.
  auto book = ref.book();
  auto chapter = ref.chapter();
  auto verse = ref.verse();
  if(verse > decltype(verse){1})
  {
    verse = reference::verse_type{verse.value - 1};
  }
  else if(chapter > decltype(chapter){1})
  {
    chapter = decltype(chapter){chapter.value - 1};
    verse = decltype(verse){verse_count(book, chapter)};
  }
  else if(util::has_prev(book))
  {
    book = util::prev(book);
    chapter = decltype(chapter){chapter_count(book)};
    verse = decltype(verse){verse_count(book, chapter)};
  }
  else
  {
    return std::nullopt;
  }
  return reference::create_unguarded(book, chapter, verse);
}

///
///
constexpr auto versification::split_by_chapter(const reference_range& ref) const -> std::vector<reference_range>
{
  const auto begin = ref.begin();
  const auto end = ref.end();
  const auto in_range = [&](const book_id book) { return begin.book() <= book && book <= end.book(); };
  const auto chapters_of = [&](const book_id book)
  {
    const auto first_chapter = book == begin.book() ? begin.chapter().value : 1U;
    const auto last_chapter = book == end.book() ? end.chapter().value : chapter_count(book);
    const auto to_range = [&, book](const std::uint32_t chapter_value)
    {
      const auto chapter = reference::chapter_type{chapter_value};
      const auto is_first = book == begin.book() && chapter == begin.chapter();
      const auto is_last = book == end.book() && chapter == end.chapter();
      return reference_range{
        reference::create_unguarded(book, chapter, is_first ? begin.verse() : reference::verse_type{1}),
        reference::create_unguarded(book, chapter, is_last ? end.verse() : reference::verse_type{verse_count(book, chapter)})
      };
    };
    return std::views::iota(first_chapter, last_chapter + 1) | std::views::transform(to_range);
  };
  return util::enum_values<book_id>() | std::views::filter(in_range) | std::views::transform(chapters_of) | std::views::join |
         std::ranges::to<std::vector>();
}

///
///
constexpr auto versification::visit_size(const data_type& data) -> std::size_t
{
  return std::visit([](const auto& d) { return d.data.size(); }, data);
}

///
///
constexpr auto versification::visit_last_reference(const data_type& data, const std::size_t index) -> std::optional<reference>
{
  return std::visit(
    [&](const auto& d) -> std::optional<reference>
    { return index < d.data.size() ? std::make_optional(d.data.at(index)) : std::nullopt; },
    data
  );
}

///
///
constexpr auto versification::visit_book_start_indices(const data_type& data) -> const book_start_indices_type&
{
  return std::visit([](const auto& d) -> const book_start_indices_type& { return d.book_start_indices; }, data);
}

///
///
constexpr auto versification::containing_index(const book_id book, const reference::chapter_type chapter) const
  -> std::optional<std::size_t>
{
  if(!util::valid(book) || chapter == decltype(chapter){0})
  {
    return std::nullopt;
  }
  const auto book_start_index = visit_book_start_indices(data_).at(util::to_integral(book));
  const auto index = book_start_index + (chapter.value - 1);
  if(const auto r = visit_last_reference(data_, index); r && r->book() == book && r->chapter() == chapter)
  {
    return index;
  }
  return std::nullopt;
}

///
///
constexpr auto versification::containing_index(const reference& ref) const -> std::optional<std::size_t>
{
  if(const auto index = containing_index(ref.book(), ref.chapter()))
  {
    const auto v = ref.verse();
    if(const auto r = visit_last_reference(data_, *index); r && v != decltype(v){0} && v <= r->verse())
    {
      return index;
    }
  }
  return std::nullopt;
}

///
/// Default versification for ESV.
///
inline constexpr auto versification_esv = versification{versification::default_esv{}};

///
/// Default versification for KJV.
///
inline constexpr auto versification_kjv = versification{versification::default_kjv{}};

///
/// Array of all default versifications.
///
inline constexpr auto versifications_default = []()
{
  using info_t = meta::pack_info<versification::all_defaults_variant>;
  return [&]<std::size_t... I>(std::index_sequence<I...>)
  { return std::array{versification{info_t::type_at<I>{}}...}; }(std::make_index_sequence<info_t::size>{});
}();

} // namespace bibstd::bible
