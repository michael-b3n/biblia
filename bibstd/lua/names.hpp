#pragma once

#include <string_view>

///
/// Global names the Lua state shares with its embedded scripts.
///
namespace bibstd::lua::names
{

inline constexpr std::string_view embedded_script_init = "init.lua";

inline constexpr std::string_view node_root = "root";
inline constexpr std::string_view node_interface = "interface";
inline constexpr std::string_view node_system = "system";
inline constexpr std::string_view node_sandbox = "sandbox";
inline constexpr std::string_view node_scripts = "scripts";
inline constexpr std::string_view node_util = "util";

inline constexpr std::string_view function_embedded = "embedded";
inline constexpr std::string_view function_log_debug = "log_debug";
inline constexpr std::string_view function_log_info = "log_info";
inline constexpr std::string_view function_log_warning = "log_warning";
inline constexpr std::string_view function_log_error = "log_error";
inline constexpr std::string_view function_get = "get";
inline constexpr std::string_view function_set = "set";
inline constexpr std::string_view function_postfix = "postfix";
inline constexpr std::string_view function_remove = "remove";

inline constexpr std::string_view value_shutdown_flag = "shutdown_flag";

inline constexpr auto script_id = std::string_view{"id"};
inline constexpr auto script_name = std::string_view{"name"};
inline constexpr auto script_functions = std::string_view{"functions"};

} // namespace bibstd::lua::names
