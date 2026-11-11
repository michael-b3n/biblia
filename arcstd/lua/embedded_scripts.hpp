#pragma once

#include <optional>
#include <span>
#include <string_view>

namespace arcstd::lua
{

///
/// Lua script compiled into the binary.
///
struct embedded_script final
{
  std::string_view name;
  std::string_view code;
};

} // namespace arcstd::lua

namespace arcstd::lua::internal
{

///
/// \return the scripts of arcstd/lua/internal, they set up the state
///
[[nodiscard]] auto all() -> std::span<const embedded_script>;

///
/// \return the script with the file name \p name
///
[[nodiscard]] auto find(std::string_view name) -> std::optional<embedded_script>;

} // namespace arcstd::lua::internal
