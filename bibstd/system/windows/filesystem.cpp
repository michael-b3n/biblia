#include "bibstd/system/filesystem.hpp"
#include "bibstd/util/exception.hpp"

#include <filesystem>

namespace bibstd::system
{

///
///
auto filesystem::local_data_folder(const std::optional<std::string_view> folder_name) -> std::filesystem::path
{
  const auto* const appdata = std::getenv("LOCALAPPDATA");
  if(appdata == nullptr)
  {
    throw util::exception("local appdata not found");
  }
  const auto folder = folder_name.has_value() ? std::filesystem::path{*folder_name} : executable_location().stem();
  return std::filesystem::path(appdata) / folder;
}

} // namespace bibstd::system
