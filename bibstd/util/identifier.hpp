#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace bibstd::util
{

///
/// Name of the characters below only and not empty, e.g. the name of a cache. So it reads the same as a key, in a
/// path and in a file name, whoever chose it. Other characters are replaced.
///
class identifier final
{
  // Variables
  std::string value_;

public: // Constants
  static constexpr auto characters = std::string_view{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"};

public: // Structors
  ///
  /// Characters of \p text other than the ones above are replaced by '_'.
  /// \throw util::exception if \p text is empty, \see from
  ///
  explicit identifier(std::string_view text);

public: // Operators
  auto operator<=>(const identifier& other) const = default;

public: // Static
  ///
  /// \return the identifier of \p text, or std::nullopt if it is empty
  ///
  [[nodiscard]] static auto from(std::string_view text) -> std::optional<identifier>;

public: // Accessors
  ///
  /// \return the name
  ///
  [[nodiscard]] auto string() const -> const std::string&;
};

} // namespace bibstd::util
