#pragma once

#include <atomic>
#include <format>

namespace bibstd::util
{

///
/// Unique ID type.
///
template<typename Tag>
class uid final
{
  // Friends
  friend struct std::formatter<uid<Tag>>;

  // Variables
  std::uint64_t value_;

public: // Typedefs
  using tag_type = Tag;
  using underlying_type = decltype(value_);

public: // Static helpers
  ///
  /// Get new typesafe unique ID.
  /// \return new typesafe unique ID
  ///
  [[nodiscard]] static auto new_uid() -> uid<tag_type>;

public: // Constructor
  uid();

public: // Operators
  constexpr auto operator<=>(const uid&) const = default;

private: // Structors
  constexpr uid(underlying_type value);
};

///
/// Get new typesafe unique ID.
/// \return new typesafe unique ID
///
template<typename Tag>
auto uid<Tag>::new_uid() -> uid<tag_type>
{
  static std::atomic_uint64_t counter{0};
  return {++counter};
}

///
///
template<typename Tag>
uid<Tag>::uid()
  : uid{new_uid()}
{
}

///
///
template<typename Tag>
constexpr uid<Tag>::uid(const underlying_type value)
  : value_{value}
{
}

} // namespace bibstd::util

///
///
template<typename T>
struct std::formatter<bibstd::util::uid<T>> : std::formatter<std::uint64_t>
{
  auto format(const bibstd::util::uid<T> id, std::format_context& ctx) const
  {
    return formatter<std::uint64_t>::format(id.value_, ctx);
  }
};
