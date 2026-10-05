#pragma once

#include "bibstd/bible/reference.hpp"

namespace bibstd::bible
{

///
/// Bible reference span class. This class contains a range of bible references.
///
class reference_range final
{
  // Variables
  reference from_;
  reference to_;

public: // Typedefs
  using chapter_type = reference::chapter_type;
  using verse_type = reference::verse_type;

public: // Constructor
  constexpr explicit reference_range(reference first_and_last);
  constexpr reference_range(reference first, reference second);

public: // Operators
  constexpr auto operator==(const reference_range&) const -> bool = default;

public: // Accessors
  ///
  /// Get the first reference in the range.
  /// \return the first reference.
  ///
  constexpr auto begin() const -> reference;

  ///
  /// Get the last reference in the range.
  /// \return the last reference.
  ///
  constexpr auto end() const -> reference;
};

///
///
constexpr reference_range::reference_range(reference first_and_last)
  : from_(first_and_last)
  , to_(first_and_last)
{
}

///
///
constexpr reference_range::reference_range(reference first, reference second)
  : from_(std::min(first, second))
  , to_(std::max(first, second))
{
}

///
///
constexpr auto reference_range::begin() const -> reference
{
  return from_;
}

///
///
constexpr auto reference_range::end() const -> reference
{
  return to_;
}

} // namespace bibstd::bible

///
///
template<>
struct std::formatter<bibstd::bible::reference_range> : std::formatter<bibstd::bible::reference>
{
  constexpr auto format(const bibstd::bible::reference_range e, std::format_context& ctx) const
  {
    return formatter<std::string>::format(std::format("{} - {}", e.begin(), e.end()), ctx);
  }
};
