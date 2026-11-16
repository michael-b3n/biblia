#pragma once

#include <filesystem>

namespace verselens
{

///
/// Show the notice that the application is already running and wait until the user closes it.
/// This is all a second instance does, it never touches what the running one owns in \p data_folder.
/// \return exit code of the application
///
[[nodiscard]] auto show_already_running(int argc, char** argv, const std::filesystem::path& data_folder) -> int;

} // namespace verselens
