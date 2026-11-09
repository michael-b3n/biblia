#pragma once

#include <optional>
#include <span>
#include <string_view>

namespace bibstd::lua
{

///
/// Lua script compiled into the binary.
///
struct embedded_script final
{
  std::string_view name;
  std::string_view code;
};

} // namespace bibstd::lua

namespace bibstd::lua::internal
{

///
/// \return the scripts of arca/lua/internal, they set up the state
///
[[nodiscard]] auto all() -> std::span<const embedded_script>;

///
/// \return the script with the file name \p name
///
[[nodiscard]] auto find(std::string_view name) -> std::optional<embedded_script>;

} // namespace bibstd::lua::internal
