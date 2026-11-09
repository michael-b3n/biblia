#pragma once

#include "arca/lua/embedded_scripts.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace bibstd::lua::bundled
{

///
/// \return the scripts of bibstd/lua/bundled and bibstd/lua/private
///
[[nodiscard]] auto all() -> std::span<const embedded_script>;

///
/// \return the script with the file name \p name
///
[[nodiscard]] auto find(std::string_view name) -> std::optional<embedded_script>;

} // namespace bibstd::lua::bundled
