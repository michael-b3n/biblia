#pragma once

#include <filesystem>

namespace verselens
{

///
/// Get the folder of the data of the application: settings, logs, scripts, scriptures and the lock of the
/// single instance. It is the folder named after the application in LOCALAPPDATA, so setting LOCALAPPDATA runs an
/// instance apart from the installed one, else in the local data location of the user.
/// \return data folder of the application
///
[[nodiscard]] auto data_folder() -> std::filesystem::path;

} // namespace verselens
