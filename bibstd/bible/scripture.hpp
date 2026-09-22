#pragma once

#include "bibstd/bible/common.hpp"
#include "bibstd/bible/reference.hpp"
#include "bibstd/bible/scripture_types.hpp"
#include "bibstd/bible/versification.hpp"

#include <map>
#include <optional>
#include <type_traits>

namespace bibstd::bible
{

///
/// A bible scripture that was read out of a bundle, see scripture_reader. It holds the passage of
/// every verse and the book names in the language of the scripture, whatever format it was read
/// from. The versification is derived from the verses the scripture actually carries.
/// \note This class is immutable and therefore thread-safe.
///
class scripture final
{
  // Variables
  const scripture_info info_;
  const std::map<book_id, book_name> book_name_data_;
  const std::map<reference, bible::passage> passage_data_;
  const bible::versification versification_;

public: // Typedefs
  using passage_map_type = std::remove_const_t<decltype(passage_data_)>;
  using book_name_map_type = std::remove_const_t<decltype(book_name_data_)>;

public: // Structors
  scripture(scripture_info info, book_name_map_type book_name_data, passage_map_type passage_data);

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
  /// Get the bible passage of the specified reference.
  /// \return Passage of the reference, or std::nullopt if the scripture does not carry it
  ///
  [[nodiscard]] auto passage(const reference& ref) const -> std::optional<bible::passage>;

  ///
  /// Get the versification associated with this scripture.
  /// \return versification
  ///
  [[nodiscard]] auto versification() const -> const bible::versification&;
};

} // namespace bibstd::bible
