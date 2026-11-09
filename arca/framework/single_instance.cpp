#include "arca/framework/single_instance.hpp"
#include "arca/system/filesystem.hpp"
#include "arca/util/exception.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <tuple>

namespace bibstd::framework
{

///
///
auto single_instance::claim(const std::string& name) -> single_instance
{
  return single_instance{name};
}

///
///
single_instance::single_instance(const std::string& name)
{
  try
  {
    const auto lock_file = system::filesystem::local_data_folder(name) / std::format("{}.lock", name);
    std::filesystem::create_directories(lock_file.parent_path());
    std::ignore = std::ofstream{lock_file, std::ios::app}; // file_lock needs an existing file, its content is never read

#ifdef BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES
    lock_.emplace(lock_file.wstring().c_str()); // the narrow overload cannot open paths outside the ansi codepage
#else
    lock_.emplace(lock_file.string().c_str());
#endif
    owner_ = lock_->try_lock();
  }
  catch(...)
  {
    owner_ = true; // an unusable lock file must not keep the application from starting
    error_ = util::exception_report();
  }
}

///
///
auto single_instance::is_owner() const -> bool
{
  return owner_;
}

///
///
auto single_instance::error() const -> const std::optional<std::string>&
{
  return error_;
}

} // namespace bibstd::framework
