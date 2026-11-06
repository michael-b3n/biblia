#pragma once

#include "bibstd/bible/common.hpp"
#include "bibstd/bible/reference.hpp"
#include "bibstd/bible/scripture_types.hpp"
#include "bibstd/bible/versification.hpp"

#include <map>
#include <optional>
#include <type_traits>
#include <vector>

namespace bibstd::bible
{

///
/// The outline of a bible scripture read from a file, see workflow::workflow_scripture_store: its information, the
/// book names and the versification, without the verses its reader keeps. A missing book takes the closest default versification.
/// \note Immutable and therefore thread-safe.
///
class scripture_outline final
{
  // Variables
  const scripture_info info_;
  const std::map<book_id, book_name> book_name_data_;
  const bible::versification versification_;

public: // Typedefs
  using book_name_map_type = std::remove_const_t<decltype(book_name_data_)>;

public: // Structors
  ///
  /// \p chapter_ends holds the last verse of each chapter. A book without chapters in it takes those of the
  /// default versification whose chapters most often end like them, e.g. the Old Testament of a New Testament.
  /// A warning names the books.
  ///
  scripture_outline(scripture_info info, book_name_map_type book_name_data, std::vector<reference> chapter_ends);

public: // Accessors
  ///
  /// Access information about the scripture.
  /// \return Information about the scripture
  ///
  [[nodiscard]] auto information() const -> const scripture_info&;

  ///
  /// Access the names of a book in the language of the scripture.
  /// \return Names of the book, or std::nullopt if the scripture does not provide them
  ///
  [[nodiscard]] auto book_information(book_id book) const -> std::optional<book_name>;

  ///
  /// Get the versification associated with this scripture.
  /// \return versification
  ///
  [[nodiscard]] auto versification() const -> const bible::versification&;
};

} // namespace bibstd::bible
