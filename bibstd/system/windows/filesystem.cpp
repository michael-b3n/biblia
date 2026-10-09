#include "bibstd/system/filesystem.hpp"
#include "bibstd/util/exception.hpp"

#include "bibstd/system/windows/win.hpp"

#include <appmodel.h>
#include <filesystem>
#include <optional>
#include <string>

namespace bibstd::system
{
namespace
{

///
/// Get the family name of the package the process runs in, e.g. as installed by the microsoft store.
/// \return package family name, std::nullopt if the process runs unpackaged or the name cannot be read
///
[[nodiscard]] auto package_family_name() -> std::optional<std::wstring>
{
  auto length = UINT32{0};
  if(GetCurrentPackageFamilyName(&length, nullptr) != ERROR_INSUFFICIENT_BUFFER)
  {
    return std::nullopt;
  }
  auto name = std::wstring(length, L'\0');
  if(GetCurrentPackageFamilyName(&length, name.data()) != ERROR_SUCCESS)
  {
    return std::nullopt;
  }
  // The length counts the terminator
  name.resize(length - 1);
  return name;
}

} // namespace

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
  // Windows redirects what a packaged process writes below appdata into its package, other processes only find it there
  if(const auto package = package_family_name())
  {
    return std::filesystem::path(appdata) / "Packages" / *package / "LocalCache" / "Local" / folder;
  }
  return std::filesystem::path(appdata) / folder;
}

} // namespace bibstd::system
