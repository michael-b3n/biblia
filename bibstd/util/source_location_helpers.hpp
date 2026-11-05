#pragma once

#include <source_location>
#include <string_view>

namespace bibstd::util
{

///
/// Get the pretty function name from the source location.
/// The pretty function name is the function name without the namespace and without the parameters.
/// \return Pretty function name as std::string_view
///
[[nodiscard]] constexpr auto filter_function_name(const std::source_location& loc) -> std::string_view
{
  const std::string_view func_name = loc.function_name();
  const auto last_braces_pos = func_name.find_first_of('(');
  const std::string_view func_name_without_braces =
    (last_braces_pos != std::string_view::npos) ? func_name.substr(0, last_braces_pos) : func_name;
  const auto colon_pos = func_name_without_braces.find_last_of(':');
  const std::string_view pretty_func_name =
    (colon_pos != std::string_view::npos) ? func_name_without_braces.substr(colon_pos + 1) : func_name_without_braces;
  return pretty_func_name;
}

///
/// Get the pretty file name from the source location.
/// The pretty file name is the file name without the path and without the file extension.
/// \return Pretty file name as std::string_view
///
[[nodiscard]] constexpr auto filter_file_name(const std::source_location& loc) -> std::string_view
{
  const std::string_view file_name = loc.file_name();
  const auto last_slash_pos = file_name.find_last_of("/\\");
  const std::string_view file_name_with_ext =
    (last_slash_pos != std::string_view::npos) ? file_name.substr(last_slash_pos + 1) : file_name;
  const auto dot_pos = file_name_with_ext.find_last_of('.');
  const std::string_view pretty_file_name =
    (dot_pos != std::string_view::npos) ? file_name_with_ext.substr(0, dot_pos) : file_name_with_ext;
  return pretty_file_name;
}

///
/// Get the pretty folder name from the source location.
/// The pretty folder name is the name of the parent folder of the file without the complete path.
/// \return Pretty folder name as std::string_view
///
[[nodiscard]] constexpr auto filter_folder_name(const std::source_location& loc) -> std::string_view
{
  const std::string_view file_name = loc.file_name();
  const auto last_slash_pos = file_name.find_last_of("/\\");
  const auto parent_folder_end = (last_slash_pos != std::string_view::npos)
                                   ? file_name.substr(0, last_slash_pos).find_last_of("/\\")
                                   : std::string_view::npos;
  const std::string_view pretty_folder_name =
    (parent_folder_end != std::string_view::npos)
      ? file_name.substr(parent_folder_end + 1, last_slash_pos - (parent_folder_end + 1))
      : "unknown";
  return pretty_folder_name;
}

} // namespace bibstd::util
