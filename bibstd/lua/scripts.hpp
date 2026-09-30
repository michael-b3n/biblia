#pragma once

#include <optional>
#include <span>
#include <string_view>

namespace bibstd::lua::scripts
{

///
/// Lua script of bibstd/lua/scripts, compiled into the binary.
///
struct script final
{
  std::string_view name;
  std::string_view code;
};

///
/// \return all scripts
///
[[nodiscard]] auto all() -> std::span<const script>;

///
/// \return the script with the file name \p name
///
[[nodiscard]] auto find(std::string_view name) -> std::optional<script>;

} // namespace bibstd::lua::scripts
