#pragma once

#include "bibstd/util/string.hpp"

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::util
{

///
/// Path of sections separated by '.', e.g. the path of a setting, a Lua node or a script manifest.
/// Empty sections are left out, so "a..b." is the path "a.b". Built from sections, each is read the same way.
///
class path final
{
  // Typedefs
  using sections_type = std::vector<std::string>;

  // Constants
  static constexpr auto delimiter = '.';

  // Variables
  sections_type sections_;

public: // Structors
  path() = default;
  path(const util::string::string_view_type auto& p);
  // Not a string, it is a range as well
  path(const std::ranges::input_range auto& sections)
    requires(!util::string::string_view_type<decltype(sections)>);

public: // Operators
  auto operator<=>(const path& other) const = default;

public: // Accessors
  ///
  /// \return the sections joined by the delimiter
  ///
  [[nodiscard]] auto string() const -> std::string;

  ///
  /// \return true if the path has no sections
  ///
  [[nodiscard]] auto empty() const -> bool;

  ///
  /// \return the sections of the path
  ///
  [[nodiscard]] auto sections() const -> const sections_type&;

  ///
  /// \return true if \p other starts with all sections of this path, also if both are equal
  ///
  [[nodiscard]] auto contains(const path& other) const -> bool;

private: // Implementation
  [[nodiscard]] static auto normalize(std::string_view p) -> sections_type;
};

///
///
path::path(const util::string::string_view_type auto& p)
  : sections_{normalize(std::string_view{p})}
{
}

///
///
path::path(const std::ranges::input_range auto& sections)
  requires(!util::string::string_view_type<decltype(sections)>)
  : sections_{normalize(util::string::join(std::ranges::to<sections_type>(sections), delimiter))}
{
}

} // namespace bibstd::util
