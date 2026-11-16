#include "src/data_folder.hpp"
#include "res/version.hpp"

#include <arcstd/util/exception.hpp>

#include <QStandardPaths>
#include <QString>
#include <QtGlobal>

namespace verselens
{

///
///
auto data_folder() -> std::filesystem::path
{
  // The environment wins over the known folder of the user, which ignores a changed LOCALAPPDATA
  auto local_data = qEnvironmentVariable("LOCALAPPDATA");
  if(local_data.isEmpty())
  {
    local_data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  }
  if(local_data.isEmpty())
  {
    throw arcstd::util::exception("local data folder not found");
  }
  return std::filesystem::path{local_data.toStdWString()} / version::data_folder_name;
}

} // namespace verselens
